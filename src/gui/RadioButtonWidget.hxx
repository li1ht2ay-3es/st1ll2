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
// Copyright (c) 1995-2017 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#ifndef RADIOBUTTON_WIDGET_HXX
#define RADIOBUTTON_WIDGET_HXX

#include "bspf.hxx"
#include "Widget.hxx"

class Dialog;
class RadioButtonGroup;

class RadioButtonWidget : public CheckboxWidget
{
  public:
    RadioButtonWidget(GuiObject* boss, const GUI::Font& font, int x, int y,
                      const string& label, RadioButtonGroup* group,
                      int cmd = 0);

    void handleMouseUp(int x, int y, int button, int clickCount) override;
    void setState(bool state, bool send = true);

  protected:
    void setFill(FillType type);
    void drawWidget(bool hilite) override;

  private:
    RadioButtonGroup* myGroup;

  private:
    // Following constructors and assignment operators not supported
    RadioButtonWidget() = delete;
    RadioButtonWidget(const RadioButtonWidget&) = delete;
    RadioButtonWidget(RadioButtonWidget&&) = delete;
    RadioButtonWidget& operator=(const RadioButtonWidget&) = delete;
    RadioButtonWidget& operator=(RadioButtonWidget&&) = delete;
};

class RadioButtonGroup
{
  public:
    RadioButtonGroup() {};

    // add widget to group
    void addWidget(RadioButtonWidget* widget);
    // tell the group which widget was selected
    void select(RadioButtonWidget* widget);
    void setSelected(uInt32 selected);
    uInt32 getSelected() { return mySelected; };

  private:
    WidgetArray myWidgets;
    uInt32 mySelected;

private:
  // Following constructors and assignment operators not supported
  RadioButtonGroup(const RadioButtonGroup&) = delete;
  RadioButtonGroup(RadioButtonGroup&&) = delete;
  RadioButtonGroup& operator=(const RadioButtonGroup&) = delete;
  RadioButtonGroup& operator=(RadioButtonGroup&&) = delete;
};

#endif
