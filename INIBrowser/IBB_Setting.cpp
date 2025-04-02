#include "FromEngine/Include.h"
#include "FromEngine/global_tool_func.h"
#include "IBBack.h"
#include "Global.h"


IBG_SettingPack GlobalSettingPack;


bool ReadSettingFileGen(const ExtFileClass& File, int Order);
bool WriteSettingFileGen(const ExtFileClass& File, int Order);

ReadFileHeader IBB_RSetting
{
    "IBB_Setting_F_200" ,
     [](const ExtFileClass& File,int FVersion,int Length)-> bool
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("从Setting.dat读入设置。");
        }
        (void)Length;
        if (FVersion > 200)
        {
            return ReadSettingFileGen(File, 0);
        }
        return false;
    }
};

WriteFileHeader IBB_WSetting
{
    "IBB_Setting_F_200" ,
     [](const ExtFileClass& File)-> bool
     {
        /*
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("调用了IBB_WSetting.Action。");
        }*/
        return WriteSettingFileGen(File, 0);
     }
};

ReadFileHeader IBB_RLastOutput
{
    "IBB_LastOutput_F_203" ,
     [](const ExtFileClass& File,int FVersion,int Length)-> bool
    {
        if (EnableLog)
        {
            GlobalLogB.AddLog_CurTime(false);
            GlobalLogB.AddLog("从Setting.dat读入上次导出目录。");
        }
        (void)Length;
        if (FVersion >= 203)
        {
            auto R = File.ReadData(IBF_Inst_Setting.List.LastOutputDir);
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog(("LastOutputDir = " + UTF8toMBCS(IBF_Inst_Setting.List.LastOutputDir)).c_str());
            }
            return R;
        }
        else
        {
            if (EnableLog)
            {
                GlobalLogB.AddLog_CurTime(false);
                GlobalLogB.AddLog("导出目录块的版本过低，读入失败。");
            }
            return false;
        }
    }
};

WriteFileHeader IBB_WLastOutput
{
    "IBB_LastOutput_F_203" ,
     [](const ExtFileClass& File)-> bool
     {
        return File.WriteData(IBF_Inst_Setting.List.LastOutputDir);
     }
};

const std::unordered_map<IBB_SettingType::_Type, std::function<bool(const IBB_SettingType&, const ExtFileClass&)>> ReadTypeMap =
{
    {IBB_SettingType::None,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        (void)Ty; (void)File; return true;
    }},
    {IBB_SettingType::IntA,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        return File.ReadData(*((int32_t*)Ty.Data));
    }},
    {IBB_SettingType::IntB,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        return File.ReadData(*((int32_t*)Ty.Data));
    }},
    {IBB_SettingType::Bool,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        return File.ReadData(*((bool*)Ty.Data));
    }},
};

const std::unordered_map<IBB_SettingType::_Type, std::function<bool(const IBB_SettingType&, const ExtFileClass&)>> WriteTypeMap =
{
    {IBB_SettingType::None,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        (void)Ty; (void)File; return true;
    }},
    {IBB_SettingType::IntA,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        return File.WriteData(*((int32_t*)Ty.Data));
    }},
    {IBB_SettingType::IntB,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        return File.WriteData(*((int32_t*)Ty.Data));
    }},
    {IBB_SettingType::Bool,[](const IBB_SettingType& Ty, const ExtFileClass& File) -> bool
    {
        return File.WriteData(*((bool*)Ty.Data));
    }},
};

extern std::vector<int> RW_ReadOrder;
extern std::vector<int> RW_WriteOrder;


void IBB_SettingRegisterRW(SaveFile& Save)
{
    Save.ReadBlockProcess[IBB_RSetting.UsingID] = IBB_RSetting;
    Save.WriteBlockProcess.push_back(IBB_WSetting);

    Save.ReadBlockProcess[IBB_RLastOutput.UsingID] = IBB_RLastOutput;
    Save.WriteBlockProcess.push_back(IBB_WLastOutput);
}

bool ReadSettingFileGen(const ExtFileClass& File, int Order)
{
    (void(Order));
    for (int Tg : RW_ReadOrder)
    {
        auto& Ty = IBF_Inst_Setting.List.Types[Tg];
        if (!ReadTypeMap.at(Ty.Type)(Ty, File))
        {
            return false;
        }
    }return true;
}
bool WriteSettingFileGen(const ExtFileClass& File, int Order)
{
    (void(Order));
    for (int Tg : RW_WriteOrder)
    {
        auto& Ty = IBF_Inst_Setting.List.Types[Tg];
        if (!WriteTypeMap.at(Ty.Type)(Ty, File))
        {
            return false;
        }
    }return true;
}


IBB_SettingTypeList::IBB_SettingTypeList()
{
    Types = {
        {
            IBB_SettingType::IntA,
                u8"帧率限制", u8"！！重启后生效\n限制使用的帧率以节约CPU\n范围15~2000\n输入-1则不限制\n默认25",
                (void*)&Pack.FrameRateLimit,
            {
                (const void*)&IBG_SettingPack::____FrameRateLimit_Def,
                (const void*)&IBG_SettingPack::____FrameRateLimit_Min,(const void*)&IBG_SettingPack::____FrameRateLimit_Max,
                (const void*)&IBG_SettingPack::____FrameRateLimit_SpV,(const void*)&IBG_SettingPack::____FrameRateLimit_SpV,
            }
        },
        {
            IBB_SettingType::IntA,
                u8"字体大小",u8"！！重启后生效\n使用的字号\n范围12~48\n默认24",
                (void*)&Pack.FontSize,
            {
                (const void*)&IBG_SettingPack::____FontSize_Def,
                (const void*)&IBG_SettingPack::____FontSize_Min,(const void*)&IBG_SettingPack::____FontSize_Max,
            }
        },
        {
            IBB_SettingType::IntA,
                u8"菜单每页条目",u8"！！重启后生效\n翻页菜单每页的条目数\n范围5~∞\n默认10",
                (void*)&Pack.MenuLinePerPage,
            {
                (const void*)&IBG_SettingPack::____MenuLinePerPage_Def,
                (const void*)&IBG_SettingPack::____MenuLinePerPage_Min,(const void*)&IBG_SettingPack::____MenuLinePerPage_Max,
            }
        },
        {
            IBB_SettingType::IntB,
                u8"拖动速率",u8"画布拖动和边缘滑动的速率基准\n范围1~6\n默认3",
                (void*)&Pack.ScrollRateLevel,
            {
                (const void*)&IBG_SettingPack::____ScrollRate_Min,//Value.Min
                (const void*)&IBG_SettingPack::____ScrollRate_Max,//Value.Max
                (const void*)u8"%d 档",//Value.Format
            }
        },
        {
            IBB_SettingType::Bool,
                u8"暗色模式",u8"决定全局主题为亮色还是暗色。\n默认为亮色模式",
                (void*)&Pack.DarkMode,
            {
                (const void*)+[]() { IBR_Color::StyleLight(); },//Action.SwitchToFalse
                (const void*)+[]() { IBR_Color::StyleDark(); }//Action.SwitchToTrue
            }
        },
        {
            IBB_SettingType::Bool,
                u8"导出后打开文件夹",u8"决定导出后是否自动打开文件夹\n默认打开",
                (void*)&Pack.OpenFolderOnOutput,
            {
                (const void*)nullptr,
                (const void*)nullptr,
            }
        },
        {
            IBB_SettingType::Bool,
                u8"保存后自动导出",u8"决定保存后是否自动导出\n默认不自动导出",
                (void*)&Pack.OutputOnSave,
            {
                (const void*)nullptr,
                (const void*)nullptr,
            }
        }
    };
}
std::vector<int> RW_ReadOrder = { 0,1,2,4,3,5,6 };
std::vector<int> RW_WriteOrder = { 0,1,2,4,3,5,6 };


void IBB_SettingTypeList::PackSetDefault()
{
    Pack.SetDefault();
    LastOutputDir.clear();
}

const IBG_SettingPack& IBG_GetSetting()
{
    return GlobalSettingPack;
}

void IBB_SetGlobalSetting(const IBG_SettingPack& Pack)
{
    GlobalSettingPack = Pack;
}

/*
版本拓展：

ReadFileHeader IBB_RSetting2
{
    "IBB_Setting_F_201" ,
     [](const ExtFileClass& File,int Version,int Length)-> bool
    {
        if (Version > 201)
        {
            return ReadSettingFileGen(File, 1);
        }
        return false;
    }
};

WriteFileHeader IBB_WSetting2
{
    "IBB_Setting_F_201" ,
     [](const ExtFileClass& File)-> bool
    {
        return ReadSettingFileGen(File, 1);
    }
};

void IBB_SettingRegisterRW(SaveFile& Save)
{
    Save.ReadBlockProcess[IBB_RSetting.UsingID] = IBB_RSetting;
    Save.WriteBlockProcess.push_back(IBB_WSetting);
    Save.ReadBlockProcess[IBB_RSetting2.UsingID] = IBB_RSetting;
    Save.WriteBlockProcess.push_back(IBB_WSetting2);
}

*/
