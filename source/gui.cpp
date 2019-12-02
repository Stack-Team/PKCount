/*
*   This file is part of PKCount
*   Copyright (C) 2019-2020 StackZ
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
*       * Requiring preservation of specified reasonable legal notices or
*         author attributions in that material or in the Appropriate Legal
*         Notices displayed by works containing it.
*       * Prohibiting misrepresentation of the origin of that material,
*         or requiring that modified versions of such material be marked in
*         reasonable ways as different from the original version.
*/


#include "gui.hpp"
#include "screens/screenCommon.hpp"

#include <3ds.h>
#include <assert.h>
#include <stack>
#include <stdarg.h>
#include <unistd.h>

C3D_RenderTarget* Top;
C3D_RenderTarget* Bottom;

static C2D_SpriteSheet sprites;

C2D_TextBuf sizeBuf;
std::stack<std::unique_ptr<Screen>> screens;

bool currentScreen = false;

void Gui::clearTextBufs(void)
{
	C2D_TextBufClear(sizeBuf);
}

Result Gui::init(void)
{
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	Top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
	Bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
	sizeBuf = C2D_TextBufNew(4096);
	sprites    = C2D_SpriteSheetLoad("romfs:/gfx/sprites.t3x");
	return 0;
}

void Gui::exit(void)
{
	C2D_SpriteSheetFree(sprites);
	C2D_TextBufDelete(sizeBuf);
	C2D_Fini();
	C3D_Fini();
}

void Gui::set_screen(C3D_RenderTarget * screen)
{
	C2D_SceneBegin(screen);
	currentScreen = screen == Top ? 1 : 0;
}

// Draw a Sprite from the Sheet.
void Gui::sprite(int key, int x, int y, float ScaleX, float ScaleY)
{
	C2D_DrawImageAt(C2D_SpriteSheetGetImage(sprites, key), x, y, 0.5f, NULL, ScaleX, ScaleY);
}

// Display a Message, which needs to be confirmed with A/B.
bool Gui::promptMsg(std::string promptMsg, std::string AOption, std::string BOption)
{
	Gui::clearTextBufs();
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C2D_TargetClear(Top, BLACK);
	C2D_TargetClear(Bottom, BLACK);
	Gui::DrawTop();
	Gui::DrawStringCentered(0, 100, 0.6f, WHITE, promptMsg.c_str(), 400);
	Gui::DrawStringCentered(0, 180, 0.6f, WHITE, "<A>: " + AOption + "   <B>: " + BOption, 400);
	Gui::DrawBottom();
	C3D_FrameEnd(0);

	while(1)
	{
		gspWaitForVBlank();
		hidScanInput();
		if(hidKeysDown() & KEY_A) {
			return true;
		} else if(hidKeysDown() & KEY_B) {
			return false;
		}
	}
}

void Gui::HelperBox(std::string Msg) {
	Gui::clearTextBufs();
	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	Gui::set_screen(Top);
	Gui::Draw_Rect(0, 0, 400, 240, C2D_Color32(0, 0, 0, 190));
	Gui::Draw_Rect(0, 25, 400, 190, C2D_Color32(150, 150, 150, 220));
	Gui::DrawString(5, 25, 0.55f, WHITE, Msg, 380);
	Gui::set_screen(Bottom);
	Gui::Draw_Rect(0, 0, 320, 240, C2D_Color32(0, 0, 0, 190));
	C3D_FrameEnd(0);
}

void Gui::DrawStringCentered(float x, float y, float size, u32 color, std::string Text, int maxWidth) {
	Gui::DrawString((currentScreen ? 200 : 160)+x-(std::min(maxWidth, (int)Gui::GetStringWidth(size, Text))/2), y, size, color, Text, maxWidth);
}

// Draw String or Text.
void Gui::DrawString(float x, float y, float size, u32 color, std::string Text, int maxWidth)
{
	C2D_Text c2d_text;
	C2D_TextParse(&c2d_text, sizeBuf, Text.c_str());


	C2D_TextOptimize(&c2d_text);
	C2D_DrawText(&c2d_text, C2D_WithColor, x, y, 0.5f, std::min(size, size*(maxWidth/Gui::GetStringWidth(size, Text))), size, color);
}


// Get String or Text Width.
float Gui::GetStringWidth(float size, std::string Text) {
	float width = 0;
	GetStringSize(size, &width, NULL, Text);
	return width;
}

// Get String or Text Size.
void Gui::GetStringSize(float size, float *width, float *height, std::string Text) {
	C2D_Text c2d_text;
	C2D_TextParse(&c2d_text, sizeBuf, Text.c_str());
	C2D_TextGetDimensions(&c2d_text, size, size, width, height);
}


// Get String or Text Height.
float Gui::GetStringHeight(float size, std::string Text) {
	float height = 0;
	GetStringSize(size, NULL, &height, Text.c_str());
	return height;
}

bool Gui::Draw_Rect(float x, float y, float w, float h, u32 color) {
	return C2D_DrawRectSolid(x, y, 0.5f, w, h, color);
}

void Gui::mainLoop(u32 hDown, u32 hHeld, touchPosition touch) {
	screens.top()->Draw();
	screens.top()->Logic(hDown, hHeld, touch);
}

void Gui::setScreen(std::unique_ptr<Screen> screen)
{
	screens.push(std::move(screen));
}

void Gui::screenBack()
{
	screens.pop();
}

// GUI.
void Gui::DrawTop(void) {
	Gui::set_screen(Top);
	Gui::Draw_Rect(0, 0, 400, 30, C2D_Color32(0, 120, 255, 255));
	Gui::Draw_Rect(0, 25, 400, 190, C2D_Color32(120, 120, 120, 255));
	Gui::Draw_Rect(0, 215, 400, 25, C2D_Color32(0, 120, 255, 255));
	Gui::sprite(sprites_top_screen_top_idx, 0, 0);
	Gui::sprite(sprites_top_screen_bot_idx, 0, 215);
}

void Gui::DrawBottom(void) {
	Gui::set_screen(Bottom);
	Gui::Draw_Rect(0, 0, 320, 30, C2D_Color32(0, 120, 255, 255));
	Gui::Draw_Rect(0, 25, 320, 190, C2D_Color32(120, 120, 120, 255));
	Gui::Draw_Rect(0, 215, 320, 25, C2D_Color32(0, 120, 255, 255));
	Gui::sprite(sprites_bottom_screen_top_idx, 0, 0);
	Gui::sprite(sprites_bottom_screen_bot_idx, 0, 215);
}