#include "Data/Image.h"
#include "ParametersTestsShared.h"

using namespace qualisys_cpp_sdk::tests;

TEST_CASE("GetImageCameraTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("GetParameters Image", qualisys_cpp_sdk::tests::data::GetImageSettingsTest, CRTPacket::PacketXML);

    bool dataAvailable = false;
    if (!protocol->ReadImageSettings(dataAvailable))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(true, dataAvailable);

    unsigned int id = 0xffffffff;
    bool enabled = false;
    CRTPacket::EImageFormat format = CRTPacket::EImageFormat::FormatRawBGR;
    unsigned int width;
    unsigned int height;
    float cropLeft;
    float cropTop;
    float cropRight;
    float cropButton;

    if (!protocol->GetImageCamera(
        0, id, enabled, format, width, height, cropLeft, cropTop, cropRight, cropButton
    )) {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(1, id);
    CHECK_EQ(true, enabled);
    CHECK_EQ(CRTPacket::EImageFormat::FormatJPG, format);
    CHECK_EQ(1920, width);
    CHECK_EQ(1088, height);
    CHECK_EQ(0.0, cropLeft);
    CHECK_EQ(0.0, cropTop);
    CHECK_EQ(1.0, cropRight);
    CHECK_EQ(1.0, cropButton);
}

TEST_CASE("SetImageSettingsTest")
{
    auto [protocol, network] = utils::CreateTestContext();

    network->PrepareResponse("<QTM_Settings>", "Setting parameters succeeded", CRTPacket::PacketCommand);

    const unsigned int nCameraID = 1u;
    const bool bEnable = true;
    const CRTPacket::EImageFormat eFormat = CRTPacket::EImageFormat::FormatRawGrayscale;
    const unsigned int nWidth = 99u;
    const unsigned int nHeight = 98u;
    const float fLeftCrop = 97.0f;
    const float fTopCrop = 96.0f;
    const float fRightCrop = 95.0f;
    const float fBottomCrop = 94.0f;

    if (!protocol->SetImageSettings(
        nCameraID, &bEnable, &eFormat,
        &nWidth, &nHeight, &fLeftCrop,
        &fTopCrop, &fRightCrop, &fBottomCrop))
    {
        FAIL(protocol->GetErrorString());
    }

    CHECK_EQ(true, utils::CompareXmlIgnoreWhitespace(qualisys_cpp_sdk::tests::data::SetImageSettingsTest, network->ReadSentData().data()));
}
