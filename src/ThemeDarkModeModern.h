/**
 * Dark mode modern theme by TheCodersCorner.com. This is part of the standard themes shipped with TcMenu.
 * This file will not be updated by the designer, you can edit.
 */
#ifndef THEME_DARK_MODE
#define THEME_DARK_MODE

#include <graphics/BaseGraphicalRenderer.h>

const color_t darkModeTitlePalette[] = {RGB(255,255,255), RGB(0,0,0), RGB(255,255,255), RGB(0,133,255)};
const color_t darkModeItemPalette[] = {RGB(255, 255, 255), RGB(0,0,0), RGB(43,43,43), RGB(65,65,65)};
const color_t darkModeActionPalette[] = {RGB(255, 255, 255), RGB(0,0,0), RGB(20,45,110), RGB(255,255,255)};

#define ACTION_BORDER_WIDTH 0

using namespace tcgfx;


void installDarkModeModernTheme(GraphicsDeviceRenderer& bgr, const MenuFontDef& itemFont, const MenuFontDef& titleFont, bool needEditingIcons) {
    // here we get a reference to the drawable and then set the dimensions.
    auto* rootDrawable = bgr.getDeviceDrawable();
    bgr.setDisplayDimensions(rootDrawable->getDisplayDimensions().x, rootDrawable->getDisplayDimensions().y);

    // we need a reference to the factory object that we will use to configure the drawing.
    auto& factory = bgr.getGraphicsPropertiesFactory();

    // change the selected colours.
    //factory.setSelectedColors(RGB(46, 66, 161), RGB(255, 255, 255));
    factory.setSelectedColors(RGB(255, 255, 255), RGB(0, 0, 0));



    // for this theme we use the same size padding for each case, we need touchable items. We calculate the height too
    MenuPadding allPadding(4, 3, 4, 3);
    int titleHeight = bgr.heightForFontPadding(titleFont.fontData, titleFont.fontMag, allPadding);
    int itemHeight = bgr.heightForFontPadding(itemFont.fontData, itemFont.fontMag, allPadding);

    // now we configure the drawing for each item type
    // Try a 30px high title?
    MenuPadding titlePadding(3,3,3,3); // try to pad the title down..
    factory.setDrawingPropertiesDefault(ItemDisplayProperties::COMPTYPE_TITLE, darkModeTitlePalette, titlePadding, titleFont.fontData, titleFont.fontMag, 3, 53,
                                        GridPosition::JUSTIFY_CENTER_NO_VALUE, MenuBorder(0));
    factory.setDrawingPropertiesDefault(ItemDisplayProperties::COMPTYPE_ITEM, darkModeItemPalette, allPadding, itemFont.fontData, itemFont.fontMag, 2, itemHeight,
                                        GridPosition::JUSTIFY_TITLE_LEFT_VALUE_RIGHT , MenuBorder(0));
    factory.setDrawingPropertiesDefault(ItemDisplayProperties::COMPTYPE_ACTION, darkModeActionPalette, allPadding, itemFont.fontData, itemFont.fontMag, 2, itemHeight,
                                        GridPosition::JUSTIFY_CENTER_WITH_VALUE, MenuBorder(ACTION_BORDER_WIDTH));


    // near the top somewhere
    factory.setEditCursorMode(CURSOR_MODE_UNDERLINE, RGB(255,0,0), RGB(255,0,0));

    // and lastly, whenever changing the configuration, we must refresh.
   ConfigurableItemDisplayPropertiesFactory::refreshCache();
}

#endif //THEME_DARK_MODE