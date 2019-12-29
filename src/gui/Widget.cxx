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
// Copyright (c) 1995-2019 by Bradford W. Mott, Stephen Anthony
// and the Stella Team
//
// See the file "License.txt" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
//   Based on code from ScummVM - Scumm Interpreter
//   Copyright (C) 2002-2004 The ScummVM project
//============================================================================

#include "bspf.hxx"
#include "Command.hxx"
#include "Dialog.hxx"
#include "FBSurface.hxx"
#include "GuiObject.hxx"
#include "OSystem.hxx"

#include "Widget.hxx"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::Widget(GuiObject* boss, const GUI::Font& font,
               int x, int y, int w, int h)
  : GuiObject(boss->instance(), boss->parent(), boss->dialog(), x, y, w, h),
    _boss(boss),
    _font(font)
{
  // Insert into the widget list of the boss
  _next = _boss->_firstWidget;
  _boss->_firstWidget = this;

  _fontWidth  = _font.getMaxCharWidth();
  _fontHeight = _font.getLineHeight();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget::~Widget()
{
  delete _next;
  _next = nullptr;

  _focusList.clear();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirty()
{
  // A widget being dirty indicates that its parent dialog is dirty
  // So we inform the parent about it
  _boss->dialog().setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::draw()
{
  if(!isVisible() || !_boss->isVisible())
    return;

  FBSurface& s = _boss->dialog().surface();

  bool onTop = _boss->dialog().isOnTop();

  bool hasBorder = _flags & Widget::FLAG_BORDER; // currently only used by Dialog widget
  int oldX = _x, oldY = _y;

  // Account for our relative position in the dialog
  _x = getAbsX();
  _y = getAbsY();

  // Clear background (unless alpha blending is enabled)
  if(_flags & Widget::FLAG_CLEARBG)
  {
    int x = _x, y = _y, w = _w, h = _h;
    if(hasBorder)
    {
      x++; y++; w-=2; h-=2;
    }
    s.fillRect(x, y, w, h, !onTop ? _bgcolorlo : (_flags & Widget::FLAG_HILITED) && isEnabled() ? _bgcolorhi : _bgcolor);
  }

  // Draw border
  if(hasBorder)
  {
    s.frameRect(_x, _y, _w, _h, !onTop ? kColor : (_flags & Widget::FLAG_HILITED) && isEnabled() ? kWidColorHi : kColor);
    _x += 4;
    _y += 4;
    _w -= 8;
    _h -= 8;
  }

  // Now perform the actual widget draw
  drawWidget((_flags & Widget::FLAG_HILITED) ? true : false);

  // Restore x/y
  if (hasBorder)
  {
    _x -= 4;
    _y -= 4;
    _w += 8;
    _h += 8;
  }

  _x = oldX;
  _y = oldY;

  // Draw all children
  Widget* w = _firstWidget;
  while(w)
  {
    w->draw();
    w = w->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::receivedFocus()
{
  if(_hasFocus)
    return;

  _hasFocus = true;
  setFlags(Widget::FLAG_HILITED);
  receivedFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::lostFocus()
{
  if(!_hasFocus)
    return;

  _hasFocus = false;
  clearFlags(Widget::FLAG_HILITED);
  lostFocusWidget();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setEnabled(bool e)
{
  if(e) setFlags(Widget::FLAG_ENABLED);
  else  clearFlags(Widget::FLAG_ENABLED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::findWidgetInChain(Widget* w, int x, int y)
{
  while(w)
  {
    // Stop as soon as we find a widget that contains the point (x,y)
    if(x >= w->_x && x < w->_x + w->_w && y >= w->_y && y < w->_y + w->_h)
      break;
    w = w->_next;
  }

  if(w)
    w = w->findWidget(x - w->_x, y - w->_y);

  return w;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Widget::isWidgetInChain(Widget* w, Widget* find)
{
  while(w)
  {
    // Stop as soon as we find the widget
    if(w == find)  return true;
    w = w->_next;
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool Widget::isWidgetInChain(WidgetArray& list, Widget* find)
{
  for(const auto& w: list)
    if(w == find)
      return true;

  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Widget* Widget::setFocusForChain(GuiObject* boss, WidgetArray& arr,
                                 Widget* wid, int direction,
                                 bool emitFocusEvents)
{
  FBSurface& s = boss->dialog().surface();
  int size = int(arr.size()), pos = -1;
  Widget* tmp;
  bool onTop = boss->dialog().isOnTop();

  for(int i = 0; i < size; ++i)
  {
    tmp = arr[i];

    // Determine position of widget 'w'
    if(wid == tmp)
      pos = i;

    // Get area around widget
    // Note: we must use getXXX() methods and not access the variables
    // directly, since in some cases (notably those widgets with embedded
    // ScrollBars) the two quantities may be different
    int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
        w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

    // First clear area surrounding all widgets
    if(tmp->_hasFocus)
    {
      if(emitFocusEvents)
        tmp->lostFocus();
      else
        tmp->_hasFocus = false;

      s.frameRect(x, y, w, h, onTop ? kDlgColor : kBGColorLo);

      tmp->setDirty();
    }
  }

  // Figure out which which should be active
  if(pos == -1)
    return nullptr;
  else
  {
    int oldPos = pos;
    do
    {
      switch(direction)
      {
        case -1:  // previous widget
          pos--;
          if(pos < 0)
            pos = size - 1;
          break;

        case +1:  // next widget
          pos++;
          if(pos >= size)
            pos = 0;
          break;

        default:
          // pos already set
          break;
      }
      // break if all widgets should be disabled
      if(oldPos == pos)
        break;
    } while(!arr[pos]->isEnabled());
  }

  // Now highlight the active widget
  tmp = arr[pos];

  // Get area around widget
  // Note: we must use getXXX() methods and not access the variables
  // directly, since in some cases (notably those widgets with embedded
  // ScrollBars) the two quantities may be different
  int x = tmp->getAbsX() - 1,  y = tmp->getAbsY() - 1,
      w = tmp->getWidth() + 2, h = tmp->getHeight() + 2;

  if(emitFocusEvents)
    tmp->receivedFocus();
  else {
    tmp->_hasFocus = true;
    tmp->setFlags(Widget::FLAG_HILITED);
  }

  if (onTop)
      s.frameRect(x, y, w, h, kWidFrameColor, FrameStyle::Dashed);

  tmp->setDirty();

  return tmp;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void Widget::setDirtyInChain(Widget* start)
{
  while(start)
  {
    start->setDirty();
    start = start->_next;
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                                   int x, int y, int w, int h,
                                   const string& text, TextAlign align,
                                   ColorId shadowColor)
  : Widget(boss, font, x, y, w, h),
    _label(text),
    _align(align)
{
  _flags = Widget::FLAG_ENABLED;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;
  _textcolor = kTextColor;
  _textcolorhi = kTextColor;
  _shadowcolor = shadowColor;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StaticTextWidget::StaticTextWidget(GuiObject* boss, const GUI::Font& font,
                                   int x, int y,
                                   const string& text, TextAlign align,
                                   ColorId shadowColor)
  : StaticTextWidget(boss, font, x, y, font.getStringWidth(text), font.getLineHeight(),
                     text, align, shadowColor)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setValue(int value)
{
  _label = std::to_string(value);

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::setLabel(const string& label)
{
  _label = label;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StaticTextWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  bool onTop = _boss->dialog().isOnTop();
  s.drawString(_font, _label, _x, _y, _w,
               isEnabled() && onTop ? _textcolor : kColor, _align, 0, true, _shadowcolor);

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int cmd, bool repeat)
  : StaticTextWidget(boss, font, x, y, w, h, label, TextAlign::Center),
    CommandSender(boss),
    _cmd(cmd),
    _repeat(repeat)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_CLEARBG;
  _bgcolor = kBtnColor;
  _bgcolorhi = kBtnColorHi;
  _bgcolorlo = kColor;
  _textcolor = kBtnTextColor;
  _textcolorhi = kBtnTextColorHi;
  _textcolorlo = kBGColorLo;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int dw,
                           const string& label, int cmd, bool repeat)
  : ButtonWidget(boss, font, x, y, font.getStringWidth(label) + dw,
                 font.getLineHeight() + 4, label, cmd, repeat)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y,
                           const string& label, int cmd, bool repeat)
  : ButtonWidget(boss, font, x, y, 20, label, cmd, repeat)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ButtonWidget::ButtonWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const uInt32* bitmap, int bmw, int bmh,
                           int cmd, bool repeat)
  : ButtonWidget(boss, font, x, y, w, h, "", cmd, repeat)
{
  _useBitmap = true;
  _bitmap = bitmap;
  _bmw = bmw;
  _bmh = bmh;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseEntered()
{
  setFlags(Widget::FLAG_HILITED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseLeft()
{
  clearFlags(Widget::FLAG_HILITED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ButtonWidget::handleEvent(Event::Type e)
{
  if(!isEnabled() || e != Event::UISelect)
    return false;

  // Simulate mouse event
  handleMouseUp(0, 0, MouseButton::LEFT, 0);
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool ButtonWidget::handleMouseClicks(int x, int y, MouseButton b)
{
  return _repeat;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(_repeat && isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(Widget::FLAG_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if (!_repeat && isEnabled() && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    clearFlags(Widget::FLAG_HILITED);
    sendCommand(_cmd, 0, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::setBitmap(const uInt32* bitmap, int bmw, int bmh)
{
  _useBitmap = true;
  _bitmap = bitmap;
  _bmh = bmh;
  _bmw = bmw;

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ButtonWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  bool onTop = _boss->dialog().isOnTop();

  s.frameRect(_x, _y, _w, _h, !onTop ? kShadowColor : hilite && isEnabled() ? kBtnBorderColorHi : kBtnBorderColor);

  if (!_useBitmap)
    s.drawString(_font, _label, _x, _y + (_h - _fontHeight)/2 + 1, _w,
                 !(isEnabled() && onTop) ? _textcolorlo :
                 hilite ? _textcolorhi : _textcolor, _align);
  else
    s.drawBitmap(_bitmap, _x + (_w - _bmw) / 2, _y + (_h - _bmh) / 2,
                 !(isEnabled() && onTop) ? _textcolorlo :
                 hilite ? _textcolorhi : _textcolor,
                 _bmw, _bmh);

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
CheckboxWidget::CheckboxWidget(GuiObject* boss, const GUI::Font& font,
                               int x, int y, const string& label,
                               int cmd)
  : ButtonWidget(boss, font, x, y, 16, 16, label, cmd)
{
  _flags = Widget::FLAG_ENABLED;
  _bgcolor = _bgcolorhi = kWidColor;
  _bgcolorlo = kDlgColor;

  _editable = true;

  if(label == "")
    _w = 14;
  else
    _w = font.getStringWidth(label) + 20;
  _h = font.getFontHeight() < 14 ? 14 : font.getFontHeight();

  // Depending on font size, either the font or box will need to be
  // centered vertically
  if(_h > 14)  // center box
    _boxY = (_h - 14) / 2;
  else         // center text
    _textY = (14 - _font.getFontHeight()) / 2;

  setFill(CheckboxWidget::FillType::Normal);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseEntered()
{
  setFlags(Widget::FLAG_HILITED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseLeft()
{
  clearFlags(Widget::FLAG_HILITED);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _editable && x >= 0 && x < _w && y >= 0 && y < _h)
  {
    toggleState();

    // We only send a command when the widget has been changed interactively
    sendCommand(_cmd, _state, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setEditable(bool editable)
{
  _editable = editable;
  if(_editable)
  {
    _bgcolor = kWidColor;
  }
  else
  {
    _bgcolor = kBGColorHi;
    setFill(CheckboxWidget::FillType::Inactive);
  }
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setFill(FillType type)
{
  /* 8x8 checkbox bitmap */
  static constexpr std::array<uInt32, 10> checked_img_active = {
    0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111,
    0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111,  0b1111111111
  };

  static constexpr std::array<uInt32, 10> checked_img_inactive = {
    0b1111111111,  0b1111111111,  0b1111001111,  0b1110000111,  0b1100000011,
    0b1100000011,  0b1110000111,  0b1111001111,  0b1111111111,  0b1111111111
  };

  static constexpr std::array<uInt32, 10> checked_img_circle = {
    0b0001111000,  0b0111111110,  0b0111111110,  0b1111111111,  0b1111111111,
    0b1111111111,  0b1111111111,  0b0111111110,  0b0111111110,  0b0001111000
  };

  switch(type)
  {
    case CheckboxWidget::FillType::Normal:
      _img = checked_img_active.data();
      _drawBox = true;
      break;
    case CheckboxWidget::FillType::Inactive:
      _img = checked_img_inactive.data();
      _drawBox = true;
      break;
    case CheckboxWidget::FillType::Circle:
      _img = checked_img_circle.data();
      _drawBox = false;
      break;
  }
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::setState(bool state, bool changed)
{
  if(_state != state)
  {
    _state = state;
    setDirty();
  }
  _changed = changed;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void CheckboxWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();
  bool onTop = _boss->dialog().isOnTop();

  if(_drawBox)
    s.frameRect(_x, _y + _boxY, 14, 14, onTop && hilite && isEnabled() && isEditable() ? kWidColorHi : kColor);
  // Do we draw a square or cross?
  s.fillRect(_x + 1, _y + _boxY + 1, 12, 12,
      _changed ? onTop ? kDbgChangedColor : kDlgColor :
      isEnabled() && onTop ? _bgcolor : kDlgColor);
  if(_state)
    s.drawBitmap(_img, _x + 2, _y + _boxY + 2, onTop && isEnabled() ? hilite && isEditable() ? kWidColorHi : kCheckColor
                 : kColor, 10);

  // Finally draw the label
  s.drawString(_font, _label, _x + 20, _y + _textY, _w,
               onTop && isEnabled() ? kTextColor : kColor);

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y, int w, int h,
                           const string& label, int labelWidth, int cmd,
                           int valueLabelWidth, const string& valueUnit, int valueLabelGap)
  : ButtonWidget(boss, font, x, y, w, h, label, cmd),
    _labelWidth(labelWidth),
    _valueUnit(valueUnit),
    _valueLabelGap(valueLabelGap),
    _valueLabelWidth(valueLabelWidth)
{
  _flags = Widget::FLAG_ENABLED | Widget::FLAG_TRACK_MOUSE;
  _bgcolor = kDlgColor;
  _bgcolorhi = kDlgColor;

  if(!_label.empty() && _labelWidth == 0)
    _labelWidth = _font.getStringWidth(_label);

  if(_valueLabelWidth == 0)
    _valueLabelGap = 0;

  _w = w + _labelWidth + _valueLabelGap + _valueLabelWidth;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SliderWidget::SliderWidget(GuiObject* boss, const GUI::Font& font,
                           int x, int y,
                           const string& label, int labelWidth, int cmd,
                           int valueLabelWidth, const string& valueUnit, int valueLabelGap)
  : SliderWidget(boss, font, x, y, font.getMaxCharWidth() * 10, font.getLineHeight(),
                 label, labelWidth, cmd, valueLabelWidth, valueUnit, valueLabelGap)
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValue(int value)
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;

  if(value != _value)
  {
    _value = value;
    setDirty();
    if (_valueLabelWidth)
      setValueLabel(_value); // update label
    sendCommand(_cmd, _value, _id);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMinValue(int value)
{
  _valueMin = value;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setMaxValue(int value)
{
  _valueMax = value;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setStepValue(int value)
{
  _stepValue = value;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValueLabel(const string& valueLabel)
{
  _valueLabel = valueLabel;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValueLabel(int value)
{
  _valueLabel = std::to_string(value);
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setValueUnit(const string& valueUnit)
{
  _valueUnit = valueUnit;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::setTickmarkIntervals(int numIntervals)
{
  _numIntervals = numIntervals;
  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseMoved(int x, int y)
{
  // TODO: when the mouse is dragged outside the widget, the slider should
  // snap back to the old value.
  if(isEnabled() && _isDragging &&
     x >= int(_labelWidth - 4) && x <= int(_w - _valueLabelGap - _valueLabelWidth + 4))
    setValue(posToValue(x - _labelWidth));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseDown(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && b == MouseButton::LEFT)
  {
    _isDragging = true;
    handleMouseMoved(x, y);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseUp(int x, int y, MouseButton b, int clickCount)
{
  if(isEnabled() && _isDragging)
    sendCommand(_cmd, _value, _id);

  _isDragging = false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::handleMouseWheel(int x, int y, int direction)
{
  if(isEnabled())
  {
    if(direction < 0)
      handleEvent(Event::UIUp);
    else if(direction > 0)
      handleEvent(Event::UIDown);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool SliderWidget::handleEvent(Event::Type e)
{
  if(!isEnabled())
    return false;

  switch(e)
  {
    case Event::UIDown:
    case Event::UILeft:
    case Event::UIPgDown:
      setValue(_value - _stepValue);
      break;

    case Event::UIUp:
    case Event::UIRight:
    case Event::UIPgUp:
      setValue(_value + _stepValue);
      break;

    case Event::UIHome:
      setValue(_valueMin);
      break;

    case Event::UIEnd:
      setValue(_valueMax);
      break;

    default:
      return false;
  }
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SliderWidget::drawWidget(bool hilite)
{
  FBSurface& s = _boss->dialog().surface();

  // Draw the label, if any
  if(_labelWidth > 0)
    s.drawString(_font, _label, _x, _y + 2, _labelWidth, isEnabled() ? kTextColor : kColor);

  int p = valueToPos(_value),
    h = _h - 10,
    x = _x + _labelWidth,
    y = _y + (_h - h) / 2 + 1;

  // Fill the box
  s.fillRect(x, y, _w - _labelWidth - _valueLabelGap - _valueLabelWidth, h,
             !isEnabled() ? kSliderBGColorLo : hilite ? kSliderBGColorHi : kSliderBGColor);
  // Draw the 'bar'
  s.fillRect(x, y, p, h,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);

  // Draw the 'tickmarks'
  for(int i = 1; i < _numIntervals; ++i)
  {
    int xt = x + (_w - _labelWidth - _valueLabelGap - _valueLabelWidth) * i / _numIntervals - 1;
    ColorId color = kNone;

    if(isEnabled())
    {
      if(xt > x + p)
        color = hilite ? kSliderColorHi : kSliderColor;
      else
        color = hilite ? kSliderBGColorHi : kSliderBGColor;
    }
    else
    {
      if(xt > x + p)
        color = kColor;
      else
        color = kSliderBGColorLo;
    }
    s.vLine(xt, y + h / 2, y + h - 1, color);
  }

  // Draw the 'handle'
  s.fillRect(x + p, y - 2, 2, h + 4,
             !isEnabled() ? kColor : hilite ? kSliderColorHi : kSliderColor);

  if(_valueLabelWidth > 0)
    s.drawString(_font, _valueLabel + _valueUnit, _x + _w - _valueLabelWidth, _y + 2,
                 _valueLabelWidth, isEnabled() ? kTextColor : kColor);

  setDirty();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::valueToPos(int value) const
{
  if(value < _valueMin)      value = _valueMin;
  else if(value > _valueMax) value = _valueMax;
  int range = std::max(_valueMax - _valueMin, 1);  // don't divide by zero

  return ((_w - _labelWidth - _valueLabelGap - _valueLabelWidth - 2) * (value - _valueMin) / range);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int SliderWidget::posToValue(int pos) const
{
  int value = (pos) * (_valueMax - _valueMin) / (_w - _labelWidth - _valueLabelGap - _valueLabelWidth - 4) + _valueMin;

  // Scale the position to the correct interval (according to step value)
  return value - (value % _stepValue);
}
