/*
 * Copyright 2003, Heikki Suhonen
 * Copyright 2009, Karsten Heimrich
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Heikki Suhonen <heikki.suhonen@gmail.com>
 *		Karsten Heimrich <host.haiku@gmx.de>
 *		Dale Cieslak <dcieslak@yahoo.com>
 *
 */

#include "TransparencyTool.h"

#include "Cursors.h"
#include "Image.h"
#include "ImageView.h"
#include "NumberSliderControl.h"
#include "PaintApplication.h"
#include "Selection.h"
#include "ToolScript.h"


#include <Bitmap.h>
#include <Catalog.h>
#include <GridLayoutBuilder.h>
#include <Window.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Tools"


using ArtPaint::Interface::NumberSliderControl;


TransparencyTool::TransparencyTool()
	: DrawingTool(B_TRANSLATE("Transparency tool"),
		TRANSPARENCY_TOOL)
{
	// The pressure option controls the speed of transparency change.
	fOptions = SIZE_OPTION | PRESSURE_OPTION | TRANSPARENCY_OPTION;
	fOptionsCount = 3;

	SetOption(SIZE_OPTION, 1);
	SetOption(PRESSURE_OPTION, 1);
	SetOption(TRANSPARENCY_OPTION, 1);
}


TransparencyTool::~TransparencyTool()
{
}


ToolScript*
TransparencyTool::UseTool(ImageView* view, uint32 buttons, BPoint point, BPoint)
{
	// Wait for the last_updated_region to become empty
	while (LastUpdatedRect().IsValid())
		snooze(50000);

	BWindow* window = view->Window();
	BBitmap* bitmap = view->ReturnImage()->ReturnActiveBitmap();

	ToolScript* the_script = new ToolScript(Type(), fToolSettings,
		((PaintApplication*)be_app)->Color(true));

	BRect bounds = bitmap->Bounds();
	uint32* bits_origin = (uint32*)bitmap->Bits();
	int32 bpr = bitmap->BytesPerRow() / 4;

	Selection* selection = view->GetSelection();

	// for the quick calculation of square-roots
	float sqrt_table[5500];
	for (int32 i=0;i<5500;i++)
		sqrt_table[i] = sqrt(i);

	float half_size = fToolSettings.size/2;
	BRect rc = BRect(floor(point.x - half_size), floor(point.y - half_size),
		ceil(point.x + half_size), ceil(point.y + half_size));

	if (selection != NULL && selection->IsEmpty() == false)
		bounds = selection->GetBoundingRect();

	rc = rc & bounds;

	SetLastUpdatedRect(rc);

	union {
		uint8 bytes[4];
		uint32 word;
	} color;

	float pressure = (float)fToolSettings.pressure / 100.;

	uint8 transparency_value =
		((100. - (float)fToolSettings.transparency) / 100.) * 255;

	while (buttons) {
		if (selection == NULL || selection->IsEmpty() == true ||
			selection->ContainsPoint(point)) {

			the_script->AddPoint(point);

			int32 x_dist,y_sqr;

			int32 width = rc.IntegerWidth();
			int32 height = rc.IntegerHeight();
			for (int32 y=0;y<height+1;y++) {
				y_sqr = (int32)(point.y - rc.top - y);
				y_sqr *= y_sqr;
				int32 real_y = (int32)(rc.top + y);
				int32 real_x;
				for (int32 x=0;x<width+1;x++) {
					x_dist = (int32)(point.x-rc.left-x);
					real_x = (int32)(rc.left+x);
					if (sqrt_table[x_dist*x_dist + y_sqr] <= half_size) {
						color.word = *(bits_origin + real_y*bpr + real_x);
						if (selection == NULL ||
							selection->IsEmpty() == true ||
							selection->ContainsPoint(real_x, real_y)) {
							uint8 diff = fabs(color.bytes[3] - transparency_value);
							uint8 step = (uint8)(ceil(diff * pressure / 2));

							if (color.bytes[3] < transparency_value) {
								color.bytes[3] = (uint8)min_c(color.bytes[3] +
									step,
									transparency_value);
								*(bits_origin + real_y*bpr + real_x) =
									color.word;
							} else if (color.bytes[3] > transparency_value) {
								color.bytes[3] = (uint8)max_c(color.bytes[3] -
									step,
									transparency_value);
								*(bits_origin + real_y*bpr + real_x) =
									color.word;
							}
						}
					}
				}
			}

			window->Lock();

			bool do_snooze = false;
			if (rc.IsValid()) {
				view->UpdateImage(rc);
				view->Sync();
				do_snooze = true;
			}

			view->getCoords(&point,&buttons);
			window->Unlock();
			half_size = fToolSettings.size/2;
			rc = BRect(floor(point.x - half_size), floor(point.y - half_size),
				ceil(point.x + half_size), ceil(point.y + half_size));
			rc = rc & bounds;
			SetLastUpdatedRect(LastUpdatedRect() | rc);
			if (do_snooze == true)
				snooze(20 * 1000);
		} else {
			window->Lock();
			view->getCoords(&point,&buttons);
			window->Unlock();
			snooze(20 * 1000);
		}
	}

	view->ReturnImage()->Render(rc);
	window->Lock();
	view->Draw(view->convertBitmapRectToView(rc));
	SetLastUpdatedRect(LastUpdatedRect() | rc);	// ???
	window->Unlock();

	return the_script;
}


int32
TransparencyTool::UseToolWithScript(ToolScript*, BBitmap*)
{
	return B_OK;
}


BView*
TransparencyTool::ConfigView()
{
	return new TransparencyToolConfigView(this);
}


const void*
TransparencyTool::ToolCursor() const
{
	return HS_TRANSPARENCY_CURSOR;
}


const char*
TransparencyTool::HelpString(bool isInUse) const
{
	return (isInUse
		? B_TRANSLATE("Adjusting the layer's transparency.")
		: B_TRANSLATE("Transparency tool"));
}


// #pragma mark -- TransparencyToolConfigView


TransparencyToolConfigView::TransparencyToolConfigView(DrawingTool* tool)
	: DrawingToolConfigView(tool)
{
	if (BLayout* layout = GetLayout()) {
		BMessage* message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", SIZE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(SIZE_OPTION));

		fSizeSlider =
			new NumberSliderControl(B_TRANSLATE("Size:"),
			"1", message, 1, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", TRANSPARENCY_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(TRANSPARENCY_OPTION));

		fTransparencySlider =
			new NumberSliderControl(B_TRANSLATE("Transparency:"),
			"1", message, 0, 100, false);

		message = new BMessage(OPTION_CHANGED);
		message->AddInt32("option", PRESSURE_OPTION);
		message->AddInt32("value", tool->GetCurrentValue(PRESSURE_OPTION));

		fSpeedSlider =
			new NumberSliderControl(B_TRANSLATE("Pressure:"),
			"1", message, 1, 100, false);

		BGridLayout* gridLayout = BGridLayoutBuilder(5.0, 5.0)
			.Add(fSizeSlider, 0, 0, 0, 0)
			.Add(fSizeSlider->LabelLayoutItem(), 0, 0)
			.Add(fSizeSlider->TextViewLayoutItem(), 1, 0)
			.Add(fSizeSlider->Slider(), 2, 0)

			.Add(fTransparencySlider, 0, 1, 0, 0)
			.Add(fTransparencySlider->LabelLayoutItem(), 0, 1)
			.Add(fTransparencySlider->TextViewLayoutItem(), 1, 1)
			.Add(fTransparencySlider->Slider(), 2, 1)

			.Add(fSpeedSlider, 0, 2, 0, 0)
			.Add(fSpeedSlider->LabelLayoutItem(), 0, 2)
			.Add(fSpeedSlider->TextViewLayoutItem(), 1, 2)
			.Add(fSpeedSlider->Slider(), 2, 2)
			.SetInsets(kWidgetInset, 0.0, 0.0, 0.0);
		gridLayout->SetMinColumnWidth(0, StringWidth("LABELSIZE"));
		gridLayout->SetMaxColumnWidth(1, StringWidth("100"));
		gridLayout->SetMinColumnWidth(2, StringWidth("SLIDERSLIDERSLIDER"));

		layout->AddView(gridLayout->View());
	}
}


void
TransparencyToolConfigView::AttachedToWindow()
{
	DrawingToolConfigView::AttachedToWindow();

	fSizeSlider->SetTarget(this);
	fSpeedSlider->SetTarget(this);
	fTransparencySlider->SetTarget(this);
}
