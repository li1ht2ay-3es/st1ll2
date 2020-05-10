//============================================================================
//
//   SSSS    tt          lll  lll
//  SS  SS   tt           ll   ll
//  SS     tttttt  eeee   ll   ll   aaaa
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-2020 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//============================================================================

#ifndef VIDEO_DIALOG_HXX
#define VIDEO_DIALOG_HXX

class CommandSender;
class CheckboxWidget;
class ColorWidget;
class DialogContainer;
class PopUpWidget;
class RadioButtonGroup;
class SliderWidget;
class StaticTextWidget;
class TabWidget;
class OSystem;

#include "Dialog.hxx"
#include "PaletteHandler.hxx"
#include "NTSCFilter.hxx"
#include "bspf.hxx"

class VideoDialog : public Dialog
{
  public:
    VideoDialog(OSystem& osystem, DialogContainer& parent, const GUI::Font& font,
                int max_w, int max_h);
    virtual ~VideoDialog() = default;

  private:
    void loadConfig() override;
    void saveConfig() override;
    void setDefaults() override;

    void addGeneralTab();
    void addPaletteTab();
    void addTVEffectsTab();
    void handleTVModeChange(NTSCFilter::Preset);
    void loadTVAdjustables(NTSCFilter::Preset preset);
    void handlePaletteChange();
    void handlePaletteUpdate();
    void handleFullScreenChange();
    void handleOverscanChange();
    void handlePhosphorChange();
    void handleCommand(CommandSender* sender, int cmd, int data, int id) override;
    void addPalette(int x, int y, int h, int w);
    void colorPalette();

  private:
    TabWidget* myTab;

    // General options
    PopUpWidget*      myRenderer{nullptr};
    SliderWidget*     myTIAZoom{nullptr};
    PopUpWidget*      myTIAPalette{nullptr};
    SliderWidget*     myPhaseShiftNtsc{nullptr};
    SliderWidget*     myPhaseShiftPal{nullptr};
    CheckboxWidget*   myTIAInterpolate{nullptr};
    SliderWidget*     myVSizeAdjust{nullptr};
    SliderWidget*     mySpeed{nullptr};

    RadioButtonGroup* myZoomGroup{nullptr};
    CheckboxWidget*   myFullscreen{nullptr};
    //PopUpWidget*      myFullScreenMode;
    CheckboxWidget*   myUseStretch{nullptr};
    SliderWidget*     myTVOverscan{nullptr};
    CheckboxWidget*   myUseVSync{nullptr};
    CheckboxWidget*   myUIMessages{nullptr};
    CheckboxWidget*   myFastSCBios{nullptr};
    CheckboxWidget*   myUseThreads{nullptr};
    std::array<StaticTextWidget*, 16> myColorLbl{nullptr};
    //std::array<ColorWidget*, 16> myColor{nullptr};
    ColorWidget*      myColor[16][8]{nullptr};

    // TV effects adjustables (custom mode)
    PopUpWidget*      myTVMode{nullptr};
    SliderWidget*     myTVSharp{nullptr};
    SliderWidget*     myTVHue{nullptr};
    SliderWidget*     myTVRes{nullptr};
    SliderWidget*     myTVArtifacts{nullptr};
    SliderWidget*     myTVFringe{nullptr};
    SliderWidget*     myTVBleed{nullptr};
    SliderWidget*     myTVBright{nullptr};
    SliderWidget*     myTVContrast{nullptr};
    SliderWidget*     myTVSatur{nullptr};
    SliderWidget*     myTVGamma{nullptr};

    // TV phosphor effect
    CheckboxWidget*   myTVPhosphor{nullptr};
    SliderWidget*     myTVPhosLevel{nullptr};

    // TV scanline intensity and interpolation
    StaticTextWidget* myTVScanLabel{nullptr};
    SliderWidget*     myTVScanIntense{nullptr};

    // TV effects adjustables presets (custom mode)
    ButtonWidget*     myCloneComposite{nullptr};
    ButtonWidget*     myCloneSvideo{nullptr};
    ButtonWidget*     myCloneRGB{nullptr};
    ButtonWidget*     myCloneBad{nullptr};
    ButtonWidget*     myCloneCustom{nullptr};

    string            myPalette;
    PaletteHandler::Adjustable myPaletteAdj{0.0F};

    enum {
      kPaletteChanged     = 'VDpl',
      kNtscShiftChanged   = 'VDns',
      kPalShiftChanged    = 'VDps',
      kPaletteUpdated     = 'VDpu',
      kSpeedupChanged     = 'VDSp',
      kVSizeChanged       = 'VDVs',
      kFullScreenChanged  = 'VDFs',
      kZoomChanged        = 'VDZo',
      kOverscanChanged    = 'VDOv',

      kTVModeChanged      = 'VDtv',
      kCloneCompositeCmd  = 'CLcp',
      kCloneSvideoCmd     = 'CLsv',
      kCloneRGBCmd        = 'CLrb',
      kCloneBadCmd        = 'CLbd',
      kCloneCustomCmd     = 'CLcu',
      kPhosphorChanged    = 'VDph',
      kPhosBlendChanged   = 'VDbl',
      kScanlinesChanged   = 'VDsc'
    };

  private:
    // Following constructors and assignment operators not supported
    VideoDialog() = delete;
    VideoDialog(const VideoDialog&) = delete;
    VideoDialog(VideoDialog&&) = delete;
    VideoDialog& operator=(const VideoDialog&) = delete;
    VideoDialog& operator=(VideoDialog&&) = delete;
};

#endif
