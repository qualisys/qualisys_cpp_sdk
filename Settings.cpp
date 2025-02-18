#include "Settings.h"

const char* qualisys_cpp_sdk::SkeletonDofToStringSettings(EDegreeOfFreedom dof)
{
    auto it = std::find_if(DEGREES_OF_FREEDOM.begin(), DEGREES_OF_FREEDOM.end(), [&](const auto& DEGREE_OF_FREEDOM) { return (DEGREE_OF_FREEDOM.first == dof); });

    if (it == DEGREES_OF_FREEDOM.end())
    {
        throw std::runtime_error("Unknown degree of freedom");
    }

    return it->second;
}

qualisys_cpp_sdk::EDegreeOfFreedom qualisys_cpp_sdk::SkeletonStringToDofSettings(const std::string& str)
{
    auto it = std::find_if(DEGREES_OF_FREEDOM.begin(), DEGREES_OF_FREEDOM.end(), [&](const auto& DEGREE_OF_FREEDOM) { return (DEGREE_OF_FREEDOM.second == str); });

    if (it == DEGREES_OF_FREEDOM.end())
    {
        throw std::runtime_error("Unknown degree of freedom");
    }

    return it->first;
}
