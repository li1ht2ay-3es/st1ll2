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

#include "Base.hxx"
#include "Font.hxx"
#include "OSystem.hxx"
#include "Debugger.hxx"
#include "RiotDebug.hxx"
#include "TIADebug.hxx"
#include "TIA.hxx"
#include "Widget.hxx"
#include "EditTextWidget.hxx"
#include "GuiObject.hxx"

#include "TiaInfoWidget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
TiaInfoWidget::TiaInfoWidget(GuiObject* boss, const GUI::Font& lfont,
                             const GUI::Font& nfont,
                             int x, int y, int max_w)
  : Widget(boss, lfont, x, y, 16, 16),
    CommandSender(boss)
{
  const int VGAP = lfont.getLineHeight() / 4;
  const int VBORDER = 5 + 1;
  const int COLUMN_GAP = _fontWidth * 1.25;
  bool longstr = lfont.getStringWidth("Frame Cycle12345") + _fontWidth * 0.5
    + COLUMN_GAP + lfont.getStringWidth("Scanline262262")
    + EditTextWidget::calcWidth(lfont) * 3 <= max_w;
  const int lineHeight = lfont.getLineHeight();
  int xpos = x, ypos = y + VBORDER;
  int lwidth = lfont.getStringWidth(longstr ? "Frame Cycls" : "F. Cycls");
  int lwidth8 = lwidth - lfont.getMaxCharWidth() * 3;
  int lwidthR = lfont.getStringWidth(longstr ? "Frame Cnt." : "Frame   ");
  int fwidth = EditTextWidget::calcWidth(lfont, 5);
  const int twidth = EditTextWidget::calcWidth(lfont, 8);
  const int LGAP = (max_w - lwidth - EditTextWidget::calcWidth(lfont, 5)
    - lwidthR - EditTextWidget::calcWidth(lfont, 5)) / 4;

  lwidth += LGAP;
  lwidth8 += LGAP;
  lwidthR += LGAP;

  // Left column
  // Left: Frame Cycle
  xpos = x;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "Frame Cycls" : "F. Cycls");
  myFrameCycles = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myFrameCycles->setEditable(false, true);

  // Left: WSync Cycles
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "WSync Cycls" : "WSync C.");
  myWSyncCylces = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myWSyncCylces->setEditable(false, true);

  // Left: Timer Cycles
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "Timer Cycls" : "Timer C.");
  myTimerCylces = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myTimerCylces->setEditable(false, true);

  // Left: Total Cycles
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, "Total");
  myTotalCycles = new EditTextWidget(boss, nfont, xpos + lwidth8, ypos - 1, twidth, lineHeight);
  myTotalCycles->setEditable(false, true);

  // Left: Delta Cycles
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, "Delta");
  myDeltaCycles = new EditTextWidget(boss, nfont, xpos + lwidth8, ypos - 1, twidth, lineHeight);
  myDeltaCycles->setEditable(false, true);

  // Right column
  xpos = x + max_w - lwidthR - EditTextWidget::calcWidth(lfont, 5); ypos = y + VBORDER;
  //xpos = myDeltaCycles->getRight() + LGAP * 2; ypos = y + VBORDER;

  // Right: Frame Count
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "Frame Cnt." : "Frame");
  myFrameCount = new EditTextWidget(boss, nfont, xpos + lwidthR, ypos - 1, fwidth, lineHeight);
  myFrameCount->setEditable(false, true);

  lwidth = lfont.getStringWidth(longstr ? "Color Clock " : "Pixel Pos ") + LGAP;
  fwidth = EditTextWidget::calcWidth(lfont, 3);

  // Right: Scanline
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "Scanline" : "Scn Ln");
  myScanlineCountLast = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myScanlineCountLast->setEditable(false, true);
  myScanlineCount = new EditTextWidget(boss, nfont,
                                       xpos + lwidth - myScanlineCountLast->getWidth() - 2, ypos - 1,
                                       fwidth, lineHeight);
  myScanlineCount->setEditable(false, true);

  // Right: Scan Cycle
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "Scan Cycle" : "Scn Cycle");
  myScanlineCycles = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myScanlineCycles->setEditable(false, true);

  // Right: Pixel Pos
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, "Pixel Pos");
  myPixelPosition = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myPixelPosition->setEditable(false, true);

  // Right: Color Clock
  ypos += lineHeight + VGAP;
  new StaticTextWidget(boss, lfont, xpos, ypos + 1, longstr ? "Color Clock" : "Color Clk");
  myColorClocks = new EditTextWidget(boss, nfont, xpos + lwidth, ypos - 1, fwidth, lineHeight);
  myColorClocks->setEditable(false, true);

  // Calculate actual dimensions
  _w = myColorClocks->getRight() - x;
  _h = myColorClocks->getBottom();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
//cerr << "TiaInfoWidget button press: x = " << x << ", y = " << y << endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::handleCommand(CommandSender* sender, int cmd, int data, int id)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void TiaInfoWidget::loadConfig()
{
  Debugger& dbg = instance().debugger();
  TIADebug& tia = dbg.tiaDebug();
  const TiaState& oldTia = static_cast<const TiaState&>(tia.getOldState());
  RiotDebug& riot = dbg.riotDebug();
  const RiotState& oldRiot = static_cast<const RiotState&>(riot.getOldState());

  myFrameCount->setText(Common::Base::toString(tia.frameCount(), Common::Base::Fmt::_10_5),
                        tia.frameCount() != oldTia.info[0]);
  myFrameCycles->setText(Common::Base::toString(tia.frameCycles(), Common::Base::Fmt::_10_5),
                         tia.frameCycles() != oldTia.info[1]);

  uInt64 total = tia.cyclesLo() + (uInt64(tia.cyclesHi()) << 32);
  uInt64 totalOld = oldTia.info[2] + (uInt64(oldTia.info[3]) << 32);
  myTotalCycles->setText(Common::Base::toString(uInt32(total) / 1000000, Common::Base::Fmt::_10_6) + "e6",
                         total != totalOld);
  uInt64 delta = total - totalOld;
  myDeltaCycles->setText(Common::Base::toString(uInt32(delta), Common::Base::Fmt::_10_8)); // no coloring

  int clk = tia.clocksThisLine();
  myScanlineCount->setText(Common::Base::toString(tia.scanlines(), Common::Base::Fmt::_10_3),
                           tia.scanlines() != oldTia.info[4]);
  myScanlineCountLast->setText(
    Common::Base::toString(tia.scanlinesLastFrame(), Common::Base::Fmt::_10_3),
    tia.scanlinesLastFrame() != oldTia.info[5]);
  myScanlineCycles->setText(Common::Base::toString(clk/3, Common::Base::Fmt::_10),
                            clk != oldTia.info[6]);
  myPixelPosition->setText(Common::Base::toString(clk-68, Common::Base::Fmt::_10),
                           clk != oldTia.info[6]);
  myColorClocks->setText(Common::Base::toString(clk, Common::Base::Fmt::_10),
                         clk != oldTia.info[6]);

  myWSyncCylces->setText(Common::Base::toString(tia.frameWsyncCycles(), Common::Base::Fmt::_10_5),
                         tia.frameWsyncCycles() != oldTia.info[7]);

  myTimerCylces->setText(Common::Base::toString(riot.timReadCycles(), Common::Base::Fmt::_10_5),
                         riot.timReadCycles() != oldRiot.timReadCycles);
}
