#include "IBB_ModProject.h"
#include "IBB_RegType.h"
#include "IBB_ModuleAlt.h"

const char* RegType_iproj_ref    = RegTypeName_iproj_ref;
const char* RegType_global_module = RegTypeName_global_module;

void IBB_ModProject::Clear()
{
    Canvas = IBB_Project{};
    SubProjects.clear();
    IsNewlyCreated = true;
    Path.clear();
    Name.clear();
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

void IBB_ModProject::InitTypes()
{
    IBB_DefaultRegType::EnsureRegType(RegTypeName_iproj_ref);
    IBB_DefaultRegType::EnsureRegType(RegTypeName_global_module);
}
