#include "Settings.h"

namespace
{
    constexpr auto DEGREES_OF_FREEDOM =
    {
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::RotationX, "RotationX"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::RotationY, "RotationY"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::RotationZ, "RotationZ"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::TranslationX, "TranslationX"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::TranslationY, "TranslationY"),
        std::make_pair(qualisys_cpp_sdk::EDegreeOfFreedom::TranslationZ, "TranslationZ")
    };
}

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
