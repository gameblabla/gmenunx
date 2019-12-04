#include "renderer.h"
#include "menu.h"
#include "constants.h"
#include "linkapp.h"
#include "rtc.h"
#include "debug.h"
#include "utilities.h"

uint8_t Renderer::getVolumeMode(uint8_t vol) {
	TRACE("getVolumeMode - enter : %i", vol);
	if (!vol) return VOLUME_MODE_MUTE;
	else if (vol > 0 && vol < 20) return VOLUME_MODE_PHONES;
	return VOLUME_MODE_NORMAL;
}

Renderer::Renderer(Esoteric *app) : 
    iconBrightness {
		app->sc->skinRes("imgs/brightness/0.png"),
		app->sc->skinRes("imgs/brightness/1.png"),
		app->sc->skinRes("imgs/brightness/2.png"),
		app->sc->skinRes("imgs/brightness/3.png"),
		app->sc->skinRes("imgs/brightness/4.png"),
		app->sc->skinRes("imgs/brightness.png"),
	}, iconBattery {
		app->sc->skinRes("imgs/battery/0.png"),
		app->sc->skinRes("imgs/battery/1.png"),
		app->sc->skinRes("imgs/battery/2.png"),
		app->sc->skinRes("imgs/battery/3.png"),
		app->sc->skinRes("imgs/battery/4.png"),
		app->sc->skinRes("imgs/battery/5.png"),
		app->sc->skinRes("imgs/battery/ac.png"),
	}, iconVolume {
		app->sc->skinRes("imgs/mute.png"),
		app->sc->skinRes("imgs/phones.png"),
		app->sc->skinRes("imgs/volume.png"),
	} {

	this->finished_ = false;
	this->app = app;
    this->rtc.refresh();

	this->prevBackdrop = app->skin->wallpaper;
	this->currBackdrop = prevBackdrop;

	iconSD = app->sc->skinRes("imgs/sd1.png");
	iconManual = app->sc->skinRes("imgs/manual.png");
	iconCPU = app->sc->skinRes("imgs/cpu.png");

	brightnessIcon = 5;
	batteryIcon = 3;
    currentVolumeMode = VOLUME_MODE_MUTE;

	helpers.clear();

	this->timerId_ = SDL_AddTimer(this->interval_, callback, this);
	this->locked_ = false;

	this->pollHW();

}

Renderer::~Renderer() {
    TRACE("~Renderer");
	this->quit();
}

void Renderer::quit() {
	this->finished_ = true;
    if (this->timerId_ > 0) {
		SDL_SetTimer(0, NULL);
        SDL_RemoveTimer(this->timerId_);
		this->interval_ = 0;
		this->timerId_ = 0;
    }
}

uint32_t Renderer::callback(uint32_t interval, void * data) {
	TRACE("enter");
	Renderer * me = static_cast<Renderer*>(data);
	if (me->finished_) {
		return (0);
	}
	me->pollHW();
	return interval;
}

void Renderer::render() {

	if (this->locked_) 
		return;
	this->locked_ = true;

    int x = 0;
    int y = 0;
    int ix = 0;
    int iy = 0;

    //TRACE("setting the clearing box");
	app->screen->box(
		(SDL_Rect){ 0, 0, app->config->resolutionX(), app->config->resolutionY() }, 
		(RGBAColor){0, 0, 0, 255});

	// do a background image or a background colour 
	if ((*app->sc)[currBackdrop]) {
		(*app->sc)[currBackdrop]->blit(app->screen,0,0);
	} else {
		app->screen->box(
			(SDL_Rect){ 0, 0, app->config->resolutionX(), app->config->resolutionY() }, 
			app->skin->colours.background);
	}

	// info bar
	//TRACE("infoBar test");
	if (app->skin->sectionInfoBarVisible) {
		if (app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_BOTTOM) {
			//TRACE("infobar needs drawing");

			SDL_Rect infoBarRect;
			switch(app->skin->sectionBar) {
				case Skin::SB_TOP:
					infoBarRect = (SDL_Rect) { 
						0, 
						app->config->resolutionY() - app->skin->sectionInfoBarSize, 
						app->config->resolutionX(), 
						app->skin->sectionInfoBarSize 
					};
					break;
				case Skin::SB_BOTTOM:
					infoBarRect = (SDL_Rect) { 
						0, 
						0, 
						app->config->resolutionX(), 
						app->skin->sectionInfoBarSize 
					};
					break;
				default:
					break;
			};

			// do we have an image
			if (!app->skin->sectionInfoBarImage.empty()) {
				//TRACE("infoBar has an image : %s", app->skin->sectionInfoBarImage.c_str());
				if ((*app->sc)[app->skin->sectionInfoBarImage]->raw->h != infoBarRect.h || (*app->sc)[app->skin->sectionInfoBarImage]->raw->w != app->config->resolutionX()) {
					//TRACE("infoBar image is being scaled");
					(*app->sc)[app->skin->sectionInfoBarImage]->softStretch(
						app->config->resolutionX(), 
						infoBarRect.h);
				}
				(*app->sc)[app->skin->sectionInfoBarImage]->blit(
					app->screen, 
					infoBarRect);
			} else {
				//TRACE("infoBar has no image, going for a simple box");
				app->screen->box(
					infoBarRect, 
					app->skin->colours.infoBarBackground);
			}

			int btnX = 6;
			int btnY = infoBarRect.y + (infoBarRect.h / 2);
			btnX = app->ui->drawButton(app->screen, "select", app->tr["edit"], btnX, btnY);
			btnX = app->ui->drawButton(app->screen, "start", app->tr["config"], btnX, btnY);
			btnX = app->ui->drawButton(app->screen, "a", app->tr["run"], btnX, btnY);
			btnX = app->ui->drawButton(app->screen, "x", app->tr["fave"], btnX, btnY);

			/*
			app->screen->write(
				app->font, 
				"\u00AB info bar \u00BB", 
				infoBarRect.w / 2, 
				infoBarRect.y + (infoBarRect.h / 2),
				HAlignCenter | VAlignMiddle);
			*/
		}
	}
		
	// SECTIONS
	//TRACE("sections");
	if (app->skin->sectionBar) {

		// do we have an image
		if (!app->skin->sectionTitleBarImage.empty()) {
			//TRACE("sectionBar has an image : %s", app->skin->sectionTitleBarImage.c_str());
			if ((*app->sc)[app->skin->sectionTitleBarImage]->raw->h != app->sectionBarRect.h || (*app->sc)[app->skin->sectionTitleBarImage]->raw->w != app->config->resolutionX()) {
				TRACE("sectionBar image is being scaled");
				(*app->sc)[app->skin->sectionTitleBarImage]->softStretch(app->config->resolutionX(), app->sectionBarRect.h);
			}
			(*app->sc)[app->skin->sectionTitleBarImage]->blit(
				app->screen, 
				app->sectionBarRect);
		} else {
			//TRACE("sectionBar has no image, going for a simple box");
			app->screen->box(app->sectionBarRect, app->skin->colours.titleBarBackground);
		}

		x = app->sectionBarRect.x; 
		y = app->sectionBarRect.y;

        //TRACE("checking mode");
		// we're in section text mode....
		if (!app->skin->showSectionIcons && (app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_BOTTOM)) {
            //TRACE("section text mode");
            string sectionName = app->menu->selSection();

            //TRACE("section text mode - writing title");
			app->screen->write(
				app->fontSectionTitle, 
				"\u00AB " + app->tr.translate(sectionName) + " \u00BB", 
				app->sectionBarRect.w / 2, 
				app->sectionBarRect.y + (app->sectionBarRect.h / 2),
				HAlignCenter | VAlignMiddle);

            //TRACE("section text mode - checking clock");
			if (app->skin->showClock) {	

                //TRACE("section text mode - writing clock");
                string clockTime = rtc.getClockTime(true);
                //TRACE("section text mode - got clock time : %s", clockTime.c_str());
				app->screen->write(
					app->fontSectionTitle, 
					clockTime, 
					4, 
					app->sectionBarRect.y + (app->sectionBarRect.h / 2),
					HAlignLeft | VAlignMiddle);
			}

		} else {
            //TRACE("icon mode");
			for (int i = app->menu->firstDispSection(); i < app->menu->getSections().size() && i < app->menu->firstDispSection() + app->menu->sectionNumItems(); i++) {
				if (app->skin->sectionBar == Skin::SB_LEFT || app->skin->sectionBar == Skin::SB_RIGHT) {
					y = (i - app->menu->firstDispSection()) * app->skin->sectionTitleBarSize;
				} else {
					x = (i - app->menu->firstDispSection()) * app->skin->sectionTitleBarSize;
				}

                //TRACE("icon mode - got x and y");
				if (app->menu->selSectionIndex() == (int)i) {
                    //TRACE("icon mode - applying highlight");
					app->screen->box(
						x, 
						y, 
						app->skin->sectionTitleBarSize, 
						app->skin->sectionTitleBarSize, 
						app->skin->colours.selectionBackground);
                }
                //TRACE("icon mode - blit");
				(*app->sc)[app->menu->getSectionIcon(i)]->blit(
					app->screen, 
					{x, y, app->skin->sectionTitleBarSize, app->skin->sectionTitleBarSize}, 
					HAlignCenter | VAlignMiddle);
			}
		}
	}

	// LINKS
	//TRACE("links");
	app->screen->setClipRect(app->linksRect);
	app->screen->box(app->linksRect, app->skin->colours.listBackground);

	int i = (app->menu->firstDispRow() * app->skin->numLinkCols);

	// single column mode
	if (app->skin->numLinkCols == 1) {
		//TRACE("column mode : %i", app->menu->sectionLinks()->size());
		// LIST
        ix = app->linksRect.x;
		for (y = 0; y < app->skin->numLinkRows && i < app->menu->sectionLinks()->size(); y++, i++) {
			iy = app->linksRect.y + y * app->linkHeight;

			// highlight selected link
			if (i == (uint32_t)app->menu->selLinkIndex())
				app->screen->box(
					ix, 
					iy, 
					app->linksRect.w, 
					app->linkHeight, 
					app->skin->colours.selectionBackground);

			int padding = 36;
			if (app->skin->linkDisplayMode == Skin::ICON_AND_TEXT || app->skin->linkDisplayMode == Skin::ICON) {
				//TRACE("theme uses icons");
				(*app->sc)[app->menu->sectionLinks()->at(i)->getIconPath()]->blit(
					app->screen, 
					{ix, iy, padding, app->linkHeight}, 
					HAlignCenter | VAlignMiddle);
			} else {
				padding = 4;
			}
				
			if (app->skin->linkDisplayMode == Skin::ICON_AND_TEXT || app->skin->linkDisplayMode == Skin::TEXT) {
				//TRACE("adding : %s", app->menu->sectionLinks()->at(i)->getDisplayTitle().c_str());
				int localXpos = ix + app->linkSpacing + padding;
				int localAlignTitle = VAlignMiddle;
				int totalFontHeight = app->fontTitle->getHeight() + app->font->getHeight();
				TRACE("total Font Height : %i, linkHeight: %i", totalFontHeight, app->linkHeight);

				if (app->skin->sectionBar == Skin::SB_BOTTOM || app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_OFF) {
					TRACE("HITTING MIDDLE ALIGN");
					localXpos = app->linksRect.w / 2;
					if (totalFontHeight >= app->linkHeight) {
						localAlignTitle = HAlignCenter | VAlignTop;
					} else {
						localAlignTitle = HAlignCenter | VAlignMiddle;
					}
				}
				app->screen->write(
					app->fontTitle, 
					app->tr.translate(app->menu->sectionLinks()->at(i)->getDisplayTitle()), 
					localXpos, 
					iy + (app->fontTitle->getHeight() / 2), 
					localAlignTitle);
					
				if (totalFontHeight < app->linkHeight) {
					app->screen->write(
						app->font, 
						app->tr.translate(app->menu->sectionLinks()->at(i)->getDescription()), 
						ix + app->linkSpacing + padding, 
						iy + app->linkHeight - (app->linkSpacing / 2), 
						VAlignBottom);
				}
			}
		}
	} else {
		//TRACE("row mode : %i", app->menu->sectionLinks()->size());
        int ix, iy = 0;
		int padding = 2;
		for (y = 0; y < app->skin->numLinkRows; y++) {
			for (x = 0; x < app->skin->numLinkCols && i < app->menu->sectionLinks()->size(); x++, i++) {

				// TRACE("getting title");
				string title = app->tr.translate(app->menu->sectionLinks()->at(i)->getDisplayTitle());
				// TRACE("got title : %s for index %i", title.c_str(), i);

				// calc cell x && y
				// TRACE("calculating heights");
				ix = app->linksRect.x + (x * app->linkWidth)  + (x + 1) * app->linkSpacing;
				iy = app->linksRect.y + (y * app->linkHeight) + (y + 1) * app->linkSpacing;

				// TRACE("setting clip rect");
				app->screen->setClipRect({ix, iy, app->linkWidth, app->linkHeight});
				Surface * icon = (*app->sc)[app->menu->sectionLinks()->at(i)->getIconPath()];

				// selected link highlight
				// TRACE("checking highlight");
				if (i == (uint32_t)app->menu->selLinkIndex()) {

					Surface *highlighter = (*app->sc)["skin:imgs/iconbg_on.png"];
					if (NULL != highlighter) {
						
						int imageX = ix;
						int imageY = iy;
						int imageWidth = app->linkWidth;
						int imageHeight = app->linkHeight;

						if (highlighter->raw->h > imageHeight || highlighter->raw->w > imageWidth) {
							if (app->skin->scaleableHighlightImage) {
								//TRACE("Highlight image is being scaled");
								highlighter->softStretch(
									imageWidth, 
									imageHeight, 
									false, 
									true);
								// dirty hack
								SDL_SetColorKey(
									highlighter->raw, 
									SDL_SRCCOLORKEY | SDL_RLEACCEL, 
									highlighter->pixel(0, 0));
							} else {
								int widthDiff = (highlighter->raw->w - imageWidth) / 2;
								int heightDiff = (highlighter->raw->h - imageHeight) / 2;

								imageX -= widthDiff;
								imageWidth = highlighter->raw->w;
								imageY -= heightDiff;
								imageHeight = highlighter->raw->h;
							}
						}

						(*app->sc)["skin:imgs/iconbg_on.png"]->blit(
							app->screen, 
							{ 
								imageX, 
								imageY, 
								imageWidth, 
								imageHeight
							}, 
							HAlignCenter | VAlignMiddle);

					} else {

						app->screen->box(
							ix, 
							iy, 
							app->linkWidth, 
							app->linkHeight, 
							app->skin->colours.selectionBackground);

					}

				}
				if (app->skin->linkDisplayMode == Skin::ICON) {
					//TRACE("adding icon and text : %s", title.c_str());
					icon->blit(
						app->screen, 
						{ ix, iy, app->linkWidth, app->linkHeight}, 
						HAlignCenter | VAlignMiddle);
				} else if (app->skin->linkDisplayMode == Skin::ICON_AND_TEXT) {
					// get the combined height
					int totalHeight = app->font->getHeight() + icon->raw->h + padding;
					// is it bigger that we have available?
					if (totalHeight > app->linkHeight) {
						// go negative padding if we need to and pull the text up
						padding = app->linkHeight - totalHeight;
						totalHeight = app->linkHeight;
					}
					int totalHalfHeight = totalHeight / 2;
					int cellHalfHeight = app->linkHeight / 2;
					int iconTop = iy + (cellHalfHeight - totalHalfHeight);
					int textTop = iconTop + icon->raw->h + padding;

					icon->blit(
						app->screen, 
						{ ix, iconTop, app->linkWidth, app->linkHeight }, 
						HAlignCenter);

					app->screen->write(app->font, 
						title, 
						ix + (app->linkWidth / 2), 
						textTop, 
						HAlignCenter);

				} else {
					//TRACE("adding text only : %s", title.c_str());
					app->screen->write(app->font, 
						title, 
						ix + (app->linkWidth / 2), 
						iy + (app->linkHeight / 2), 
						HAlignCenter | VAlignMiddle);
				}

			}
		}
	}
	//TRACE("done");
	app->screen->clearClipRect();

	// add a scroll bar if we're not in single row world
	if (this->app->skin->numLinkRows > 1) {
		app->ui->drawScrollBar(app->skin->numLinkRows, 
			app->menu->sectionLinks()->size() / app->skin->numLinkCols + ((app->menu->sectionLinks()->size() % app->skin->numLinkCols==0) ? 0 : 1), 
			app->menu->firstDispRow(), 
			app->linksRect);
	}

	currBackdrop = app->skin->wallpaper;
	if (app->menu->selLink() != NULL && app->menu->selLinkApp() != NULL && !app->menu->selLinkApp()->getBackdropPath().empty() && app->sc->addImage(app->menu->selLinkApp()->getBackdropPath()) != NULL) {
		TRACE("setting currBackdrop to : %s", app->menu->selLinkApp()->getBackdropPath().c_str());
		currBackdrop = app->menu->selLinkApp()->getBackdropPath();
	}

	//Background has changed flip it and return out quickly
	if (prevBackdrop != currBackdrop) {
		INFO("New backdrop: %s", currBackdrop.c_str());
		app->sc->del(prevBackdrop);
		prevBackdrop = currBackdrop;
		// input.setWakeUpInterval(1);

		this->locked_ = false;
		return;
	}

	/* 
	 *
	 * helper icon section
	 * 
	 */
	if (app->skin->sectionBar) {
		// tray helper icons
		int helperHeight = 20;
		int maxItemsPerRow = 0;
		if (app->sectionBarRect.w > app->sectionBarRect.h) {
			maxItemsPerRow = (int)(app->sectionBarRect.h / (float)helperHeight);
		} else {
			maxItemsPerRow = (int)(app->sectionBarRect.w / (float)helperHeight);
		}
		int rootXPos = app->sectionBarRect.x + app->sectionBarRect.w - 18;
		int rootYPos = app->sectionBarRect.y + app->sectionBarRect.h - 18;
		//TRACE("hitting up the helpers");

		helpers.push_back(iconVolume[currentVolumeMode]);
		helpers.push_back(iconBattery[batteryIcon]);
		if (app->hw->getCardStatus() == IHardware::MMC_MOUNTED) {
			helpers.push_back(iconSD);
		}
		helpers.push_back(iconBrightness[brightnessIcon]);
		if (app->menu->selLink() != NULL) {
			if (app->menu->selLinkApp() != NULL) {
				if (!app->menu->selLinkApp()->getManualPath().empty()) {
					// Manual indicator
					helpers.push_back(iconManual);
				}
				if (app->menu->selLinkApp()->clock() != app->config->cpuMenu()) {
					// CPU indicator
					helpers.push_back(iconCPU);
				}
			}
		}
		//TRACE("layoutHelperIcons");
		int * xPosPtr = & rootXPos;
		int * yPosPtr = & rootYPos;
		layoutHelperIcons(helpers, app->screen, helperHeight, xPosPtr, yPosPtr, maxItemsPerRow);
		//TRACE("helpers.clear()");
		helpers.clear();

		if (app->skin->showClock) {
			if (app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_BOTTOM) {
				if (app->skin->showSectionIcons) {
					// grab the new x offset and write the clock
					string time = rtc.getClockTime(true);
					app->screen->write(
						app->fontSectionTitle, 
						time, 
						*(xPosPtr) - (app->fontSectionTitle->getTextWidth(time) / 2), 
						app->sectionBarRect.y + (app->sectionBarRect.h / 2),
						VAlignMiddle);
				}
			} else {
				// grab the new y offset and write the clock
				app->screen->write(
					app->fontSectionTitle, 
					rtc.getClockTime(true), 
					app->sectionBarRect.x + 4, 
					*(yPosPtr),
					HAlignLeft | VAlignTop);
			}
		}
	} // app->skin->sectionBar

    //TRACE("flip"); 
	app->screen->flip();
	this->locked_ = false;
    TRACE("exit");
}

void Renderer::layoutHelperIcons(vector<Surface*> icons, Surface *target, int helperHeight, int * rootXPosPtr, int * rootYPosPtr, int iconsPerRow) {
	TRACE("enter - rootXPos = %i, rootYPos = %i", *rootXPosPtr, *rootYPosPtr);
	int iconCounter = 0;
	int currentXOffset = 0;
	int currentYOffset = 0;
	int rootXPos = *rootXPosPtr;
	int rootYPos = *rootYPosPtr;

	for(std::vector<Surface*>::iterator it = icons.begin(); it != icons.end(); ++it) {
		TRACE("blitting");
		(*it)->blit(
			app->screen, 
			rootXPos - (currentXOffset * (helperHeight - 2)), 
			rootYPos - (currentYOffset * (helperHeight - 2))
		);
		if (++iconCounter % iconsPerRow == 0) {
			++currentXOffset;
			currentYOffset = 0;
		} else {
			++currentYOffset;
		}
	};
	*rootXPosPtr = rootXPos - (currentXOffset * (helperHeight - 2));
	*rootYPosPtr = rootYPos - (currentXOffset * (helperHeight - 2));
	TRACE("exit - rootXPos = %i, rootYPos = %i", *rootXPosPtr, *rootYPosPtr);
}

void Renderer::pollHW() {
	// if we're going to draw helpers, get their latest value
	TRACE("section bar test");
	if (this->app->skin->sectionBar) {
		TRACE("section bar exists in skin settings");
		TRACE("updating helper icon status");
		this->batteryIcon = this->app->hw->getBatteryLevel();
		if (this->batteryIcon > 5) this->batteryIcon = 6;

		this->brightnessIcon = this->app->hw->getBacklightLevel();
		if (this->brightnessIcon > 4 || this->iconBrightness[this->brightnessIcon] == NULL) 
			this->brightnessIcon = 5;

		int currentVolume = this->app->hw->getVolumeLevel();
		this->currentVolumeMode = this->getVolumeMode(currentVolume);
        this->rtc.refresh();
		TRACE("helper icon status updated");
    }
	if (this->app->cache->isDirty()) {
		this->app->initMenu();
	}
}

