#include "IBB_ModProject.h"
#include "IBB_RegType.h"
#include "IBB_ModuleAlt.h"
#include "FromEngine/global_tool_func.h"
#include "Global.h"
#include "IBSave.h"
#include <functional>
#include <set>

const char* RegType_iproj_ref    = RegTypeName_iproj_ref;
const char* RegType_global_module = RegTypeName_global_module;

void IBB_ModProject::Clear()
{
    Canvas = IBB_Project{};
    SubProjects.clear();
    IsNewlyCreated = true;
    Path.clear();
    Name.clear();
    BuildOutputDir.clear();
    BuildLog.clear();
    CompiledIniFiles.clear();
}

void IBB_ModProject::AddSubProject(const std::wstring& iproj_path,
                                   const std::string& display_name,
                                   const std::string& export_prefix)
{
    IBB_SubProjectRef ref;
    ref.iproj_path = iproj_path;
    ref.display_name = display_name;
    ref.export_prefix = export_prefix;
    SubProjects.push_back(ref);
}

// ---- Binary metadata tail ----
// After ClipBoardData in IBS_Inst_Project.Data, append "MEBD" + KV pairs.
// SetStream() stops after parsing all modules; the tail is ignored.

static constexpr uint32_t META_MAGIC = 'M' | ('E' << 8) | ('B' << 16) | ('D' << 24);

void IBB_ModProject::WriteMetaTail(std::vector<BYTE>& data, const std::unordered_map<std::string, std::string>& kv)
{
    if (kv.empty())
    {
        // 写入空尾缀：magic + 0 keys
        BYTE magic[4] = { 'M','E','B','D' };
        data.insert(data.end(), magic, magic + 4);
        uint32_t n = 0;
        data.insert(data.end(), (BYTE*)&n, (BYTE*)&n + 4);
        return;
    }
    BYTE magic[4] = { 'M','E','B','D' };
    data.insert(data.end(), magic, magic + 4);
    uint32_t numKeys = (uint32_t)kv.size();
    auto p = (BYTE*)&numKeys;
    data.insert(data.end(), p, p + 4);
    for (auto& [k, v] : kv)
    {
        uint32_t kl = (uint32_t)k.size();
        p = (BYTE*)&kl; data.insert(data.end(), p, p + 4);
        data.insert(data.end(), k.begin(), k.end());
        uint32_t vl = (uint32_t)v.size();
        p = (BYTE*)&vl; data.insert(data.end(), p, p + 4);
        data.insert(data.end(), v.begin(), v.end());
    }
}

std::unordered_map<std::string, std::string> IBB_ModProject::ReadMetaTail(const std::vector<BYTE>& data)
{
    std::unordered_map<std::string, std::string> result;
    if (data.size() < 8) return result;
    // 从尾部搜索 magic "MEBD"
    size_t pos = data.size();
    // search backwards for the last occurrence of magic
    for (size_t i = data.size() - 4; i != (size_t)-1; i--)
    {
        if (data[i] == 'M' && data[i + 1] == 'E' && data[i + 2] == 'B' && data[i + 3] == 'D')
        {
            pos = i;
            break;
        }
    }
    if (pos >= data.size() - 4) return result; // not found
    size_t off = pos + 4;
    if (off + 4 > data.size()) return result;
    uint32_t numKeys;
    memcpy(&numKeys, &data[off], 4);
    off += 4;
    for (uint32_t i = 0; i < numKeys; i++)
    {
        if (off + 4 > data.size()) break;
        uint32_t kl; memcpy(&kl, &data[off], 4); off += 4;
        if (off + kl > data.size()) break;
        std::string key((const char*)&data[off], kl); off += kl;
        if (off + 4 > data.size()) break;
        uint32_t vl; memcpy(&vl, &data[off], 4); off += 4;
        if (off + vl > data.size()) break;
        std::string val((const char*)&data[off], vl); off += vl;
        result[key] = val;
    }
    GlobalLogB.AddLog((std::string("DBG[MetaTail] Read: numKeys=") + std::to_string(numKeys)
        + " pos=" + std::to_string(pos)).c_str());
    return result;
}

// ---- iproj ModProjPath metadata ops (binary tail version) ----
// Read/write ModProjPath KV pairs at the tail of .iproj Data.

static void IprojMetaOp(const std::wstring& iprojPath,
    const std::function<void(std::unordered_map<std::string, std::string>& meta)>& op)
{
    GlobalLogB.AddLog((std::string("DBG[MetaOp] ENTER: ") + UnicodetoUTF8(iprojPath)).c_str());
    if (GetFileAttributesW(iprojPath.c_str()) == INVALID_FILE_ATTRIBUTES)
    {
        GlobalLogB.AddLog("DBG[MetaOp] SKIP: file not found");
        return;
    }
    auto origData = std::move(IBS_Inst_Project.Data);
    auto origPath = std::move(IBS_Inst_Project.Path);
    auto origOutDir = std::move(IBS_Inst_Project.LastOutputDir);
    auto origOutIni = std::move(IBS_Inst_Project.LastOutputIniName);
    GlobalLogB.AddLog((std::string("DBG[MetaOp] SAVED_STATE: origDataSz=") + std::to_string(origData.size())
        + " origPath=" + UnicodetoUTF8(origPath)).c_str());
    IBS_Inst_Project.Path = iprojPath;
    IBS_Inst_Project.LastOutputDir.clear();
    IBS_Inst_Project.LastOutputIniName.clear();
    if (!IBS_Inst_Project.Load()) { GlobalLogB.AddLog("DBG[MetaOp] Load FAILED"); goto restore; }
    {
        GlobalLogB.AddLog((std::string("DBG[MetaOp] Load OK: IBS_DataSz=") + std::to_string(IBS_Inst_Project.Data.size())).c_str());
        // Read existing metadata
        auto meta = IBB_ModProject::ReadMetaTail(IBS_Inst_Project.Data);
        GlobalLogB.AddLog((std::string("DBG[MetaOp] MetaTail keys=") + std::to_string(meta.size())).c_str());
        // Call operation
        op(meta);
        // Rebuild ClipBoardData to strip old tail, then append new tail
        IBB_ClipBoardData clip;
        if (clip.SetStream(IBS_Inst_Project.Data,
            IBS_Inst_Project.GetCreateVersionN() / 10000 * 10000
            + IBS_Inst_Project.GetCreateVersionN() % 10000 / 100 * 100
            + IBS_Inst_Project.GetCreateVersionN() % 100))
        {
            GlobalLogB.AddLog((std::string("DBG[MetaOp] SetStream OK: clipModules=") + std::to_string(clip.Modules.size())).c_str());
        }
        else
        {
            GlobalLogB.AddLog("DBG[MetaOp] SetStream FAILED, writing meta to raw Data");
        }
        IBS_Inst_Project.Data = clip.GetStream();
        IBB_ModProject::WriteMetaTail(IBS_Inst_Project.Data, meta);
        GlobalLogB.AddLog((std::string("DBG[MetaOp] PRE_SAVE: newDataSz=") + std::to_string(IBS_Inst_Project.Data.size())).c_str());
        IBS_Inst_Project.Save();
        GlobalLogB.AddLog("DBG[MetaOp] Save DONE");
    }
restore:
    IBS_Inst_Project.Data = std::move(origData);
    IBS_Inst_Project.Path = std::move(origPath);
    IBS_Inst_Project.LastOutputDir = std::move(origOutDir);
    IBS_Inst_Project.LastOutputIniName = std::move(origOutIni);
    GlobalLogB.AddLog((std::string("DBG[MetaOp] RESTORED: DataSz=") + std::to_string(IBS_Inst_Project.Data.size())
        + " Path=" + UnicodetoUTF8(IBS_Inst_Project.Path)).c_str());
}

void IBB_ModProject::AddIprojModProjPath(const std::wstring& iprojPath, const std::wstring& modprojPath)
{
    IprojMetaOp(iprojPath, [&](std::unordered_map<std::string, std::string>& meta) {
        auto modUtf8 = UnicodetoUTF8(modprojPath);
        auto& existing = meta[ModProjKey];
        // split, deduplicate, rejoin
        std::set<std::string> paths;
        if (!existing.empty())
        {
            size_t p = 0, q;
            while ((q = existing.find('\n', p)) != std::string::npos)
            {
                if (q > p) paths.insert(existing.substr(p, q - p));
                p = q + 1;
            }
            if (p < existing.size()) paths.insert(existing.substr(p));
        }
        if (paths.insert(modUtf8).second)
        {
            existing.clear();
            for (auto& s : paths) { if (!existing.empty()) existing += '\n'; existing += s; }
        }
    });
}

void IBB_ModProject::RemoveIprojModProjPath(const std::wstring& iprojPath, const std::wstring& modprojPath)
{
    IprojMetaOp(iprojPath, [&](std::unordered_map<std::string, std::string>& meta) {
        auto it = meta.find(ModProjKey);
        if (it == meta.end()) return;
        auto modUtf8 = UnicodetoUTF8(modprojPath);
        std::set<std::string> paths;
        auto& existing = it->second;
        size_t p = 0, q;
        while ((q = existing.find('\n', p)) != std::string::npos)
        {
            if (q > p) paths.insert(existing.substr(p, q - p));
            p = q + 1;
        }
        if (p < existing.size()) paths.insert(existing.substr(p));
        if (paths.erase(modUtf8))
        {
            if (paths.empty())
                meta.erase(it);
            else
            {
                existing.clear();
                for (auto& s : paths) { if (!existing.empty()) existing += '\n'; existing += s; }
            }
        }
    });
}

std::vector<std::wstring> IBB_ModProject::GetIprojModProjPaths(const std::wstring& iprojPath)
{
    std::vector<std::wstring> result;
    IprojMetaOp(iprojPath, [&](std::unordered_map<std::string, std::string>& meta) {
        auto it = meta.find(ModProjKey);
        if (it == meta.end()) return;
        auto& v = it->second;
        if (v.empty()) return;
        size_t p = 0, q;
        while ((q = v.find('\n', p)) != std::string::npos)
        {
            if (q > p) result.push_back(UTF8toUnicode(v.substr(p, q - p)));
            p = q + 1;
        }
        if (p < v.size()) result.push_back(UTF8toUnicode(v.substr(p)));
    });
    return result;
}

void IBB_ModProject::InitTypes()
{
    IBB_DefaultRegType::EnsureRegType(RegTypeName_iproj_ref);
    IBB_DefaultRegType::EnsureRegType(RegTypeName_global_module);
}
