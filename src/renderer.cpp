#include "renderer.h"
#include "menu.h"
#include "constants.h"
#include "linkapp.h"
#include "debug.h"
#include "utilities.h"

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

	this->iconSD = app->sc->skinRes("imgs/sd1.png");
	this->iconManual = app->sc->skinRes("imgs/manual.png");
	this->iconCPU = app->sc->skinRes("imgs/cpu.png");
	this->highlighter = (*app->sc)["skin:imgs/iconbg_on.png"];

	this->brightnessIcon = 5;
	this->batteryIcon = 3;
    this->currentVolumeMode = VOLUME_MODE_MUTE;

	this->interval_ = 2000;
	this->timerId_ = 0;
	this->locked_ = false;

	this->helpers.clear();
	this->backgrounds = new SurfaceCollection(this->app->skin, false);
	this->pollHW();

}

Renderer::~Renderer() {
    TRACE("~Renderer");
	this->quit();
}

void Renderer::startPolling() {
	TRACE("enter");
	if (this->timerId_ != 0)  {
		TRACE("we already have a timer running");
		return;
	}
	this->timerId_ = SDL_AddTimer(this->interval_, callback, this);
	TRACE("timer started with id : %lu", (long)this->timerId_);
}

void Renderer::stopPolling() {
	TRACE("enter - timer id : %lu", (long)this->timerId_);
    if (this->timerId_ > 0) {
		//SDL_SetTimer(0, NULL);
        SDL_RemoveTimer(this->timerId_);
		this->timerId_ = 0;
    }
	TRACE("exit - timer id : %lu", (long)this->timerId_);
}

void Renderer::quit() {
	TRACE("enter");
	this->finished_ = true;
	this->stopPolling();

	std::vector<Surface *>::iterator helperIt = this->helpers.begin();
	while (helperIt != this->helpers.end()) {
        helperIt = this->helpers.erase(helperIt);
        TRACE("remaining helper collection size : %zu", this->helpers.size());
	}
	this->helpers.clear();

	if (nullptr != this->backgrounds)
		delete this->backgrounds;

	TRACE("exit");
}

void Renderer::render() {
	TRACE("enter");
	if (this->locked_ || this->finished_) 
		return;
	this->locked_ = true;

    int x = 0;
    int y = 0;
    int ix = 0;
    int iy = 0;
	int screenX = this->app->getScreenWidth();
	int screenY = this->app->getScreenHeight();
	// we only care about an info bar if the skin says it's on
	// and we're in top or bottom mode
	bool infoBarIsInPlay = app->skin->sectionInfoBarVisible && (app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_BOTTOM);

	if (!app->skin->wallpaper.empty()) {
		// blit the bg
		(*app->sc)[app->skin->wallpaper]->blit(app->screen, 0, 0);
	} else {
		// colour only
		app->screen->box(
			SDL_Rect { 0, 0, screenX, screenY },
			app->skin->colours.background);
	}

	// now check for an override
	// if it's fullscreen just blit it
	// but clip to links rect, so colour is preserved
	// else scale it once and put it back in the sc
	// we can't use the app sc as we're messing with previews already loaded

	if (this->app->skin->skinBackdrops) {
		if (app->menu->selLinkApp() != NULL) {
			std::string backdropKey = app->menu->selLinkApp()->getBackdropPath();
			if (!backdropKey.empty()) {
				if (NULL != (*this->backgrounds)[backdropKey]) {
					TRACE("adding background : '%s'", backdropKey.c_str());
					// good to scale and blit
					int displayX = 0;
					int displayY = 0;
					int myW = (*this->backgrounds)[backdropKey]->raw->w;
					int myH = (*this->backgrounds)[backdropKey]->raw->h;

					// test for non fullscreen fit
					if (myW != screenX && myH != screenY) {
						// it's not a fullscreen image, so we want to make it best fit in the link rect
						// but we only want to stretch it once, at most
						int margin = 5;
						
						if (myW > (app->linksRect.w - margin) || myH > (app->linksRect.h - margin)) {
							(*this->backgrounds)[backdropKey]->softStretch(
									app->linksRect.w - margin, 
									app->linksRect.h - margin, 
									true);

							myW = (*this->backgrounds)[backdropKey]->raw->w;
							myH = (*this->backgrounds)[backdropKey]->raw->h;
						}
						myW /= 2;
						myH /= 2;
						int offset = 0;
						if (app->skin->sectionBar == Skin::SB_LEFT || app->skin->sectionBar == Skin::SB_RIGHT) {
							offset = (app->sectionBarRect.w / 2);
						}
						displayX = (screenX / 2) - myW + offset;
						displayY = (screenY / 2) - myH;

					}
					// and finally blit it
					(*this->backgrounds)[backdropKey]->blit(app->screen, displayX, displayY);
				}
			}
		}
	}

	// info bar
	//TRACE("infoBar test");
	if (infoBarIsInPlay) 	{
		SDL_Rect infoBarRect;
		switch (app->skin->sectionBar) 		{
		case Skin::SB_TOP:
			infoBarRect = (SDL_Rect){
				0,
				screenY - app->skin->sectionInfoBarSize,
				screenX,
				app->skin->sectionInfoBarSize};
			break;
		case Skin::SB_BOTTOM:
			infoBarRect = (SDL_Rect){
				0,
				0,
				screenX,
				app->skin->sectionInfoBarSize};
			break;
		default:
			break;
		};

		// do we have an image
		if (!app->skin->sectionInfoBarImage.empty() && NULL != (*app->sc)[app->skin->sectionInfoBarImage]) {
			//TRACE("infoBar has an image : %s", app->skin->sectionInfoBarImage.c_str());
			if ((*app->sc)[app->skin->sectionInfoBarImage]->raw->h != infoBarRect.h || (*app->sc)[app->skin->sectionInfoBarImage]->raw->w != screenX) {
				//TRACE("infoBar image is being scaled");
				(*app->sc)[app->skin->sectionInfoBarImage]->softStretch(
					screenX,
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

		// show exact battery %
		if (!this->app->skin->showBatteryIcons) {
			std::string batteryLevel = app->hw->Power()->displayLevel();
			app->screen->write(
				app->font,
				batteryLevel,
				btnX + 4,
				btnY,
				HAlignLeft | VAlignMiddle);
		}
	}

	// SECTIONS
	//TRACE("sections");
	if (app->skin->sectionBar) {

		// do we have an image
		if (!app->skin->sectionTitleBarImage.empty() && NULL != (*app->sc)[app->skin->sectionTitleBarImage]) {
			//TRACE("sectionBar has an image : %s", app->skin->sectionTitleBarImage.c_str());
			if ((*app->sc)[app->skin->sectionTitleBarImage]->raw->h != app->sectionBarRect.h || (*app->sc)[app->skin->sectionTitleBarImage]->raw->w != screenX) {
				TRACE("sectionBar image is being scaled");
				(*app->sc)[app->skin->sectionTitleBarImage]->softStretch(screenX, app->sectionBarRect.h);
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
            std::string sectionName = app->menu->selSection();

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
                std::string clockTime = app->hw->Clock()->getClockTime(true);
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
					{ x, y, app->skin->sectionTitleBarSize, app->skin->sectionTitleBarSize }, 
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
				Surface * sfcIcon = (*app->sc)[app->menu->sectionLinks()->at(i)->getIconPath()];
				if (NULL != sfcIcon) {
					sfcIcon->blit(
						app->screen, 
						{ ix, iy, padding, app->linkHeight }, 
						HAlignCenter | VAlignMiddle);
				} else {
					WARNING("couldn't load expected icon : '%s'", app->menu->sectionLinks()->at(i)->getIconPath().c_str());
				}
			} else {
				padding = 4;
			}
				
			if (app->skin->linkDisplayMode == Skin::ICON_AND_TEXT || app->skin->linkDisplayMode == Skin::TEXT) {
				//TRACE("adding : %s", app->menu->sectionLinks()->at(i)->getDisplayTitle().c_str());
				int localXpos = ix + app->linkSpacing + padding;
				int localAlignTitle = VAlignMiddle;
				int totalFontHeight = app->fontTitle->getHeight() + app->font->getHeight();
				//TRACE("total Font Height : %i, linkHeight: %i", totalFontHeight, app->linkHeight);

				if (app->skin->sectionBar == Skin::SB_BOTTOM || app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_OFF) {
					//TRACE("HITTING MIDDLE ALIGN");
					localXpos = app->linksRect.w / 2;
					localAlignTitle = HAlignCenter | VAlignMiddle;
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
		TRACE("row mode : %zu", app->menu->sectionLinks()->size());
        int ix, iy = 0;
		int padding = 2;
		for (y = 0; y < app->skin->numLinkRows; y++) {
			for (x = 0; x < app->skin->numLinkCols && i < app->menu->sectionLinks()->size(); x++, i++) {

				//TRACE("getting title");
				std::string title = app->tr.translate(app->menu->sectionLinks()->at(i)->getDisplayTitle());
				//TRACE("got title : %s for index %i", title.c_str(), i);

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

					if (NULL != this->highlighter) {
						
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

						this->highlighter->blit(
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
						{ ix, iy, app->linkWidth, app->linkHeight }, 
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


	/* 
	 *
	 * helper icon section
	 * 
	 */
	if (app->skin->sectionBar) {
		int rootXPos = app->sectionBarRect.x + app->sectionBarRect.w - 18;
		int rootYPos = app->sectionBarRect.y + app->sectionBarRect.h - 18;

		// clock
		if (app->skin->showClock) {

			std::string time = this->app->hw->Clock()->getClockTime(true);

			if (app->skin->sectionBar == Skin::SB_TOP || app->skin->sectionBar == Skin::SB_BOTTOM) {
				if (app->skin->showSectionIcons) {
					// grab the new x offset and write the clock
					
					app->screen->write(
						app->fontSectionTitle, 
						time, 
						rootXPos - (app->fontSectionTitle->getTextWidth(time) / 2), 
						app->sectionBarRect.y + (app->sectionBarRect.h / 2),
						VAlignMiddle);
					
					// recalc the x pos on root
					rootXPos -= (app->fontSectionTitle->getTextWidth("00:00") + 4);
				}
			} else {
				// grab the new y offset and write the clock
				app->screen->write(
					app->fontSectionTitle, 
					time, 
					app->sectionBarRect.x + 4, 
					rootYPos,
					HAlignLeft | VAlignTop);

				// recalc the y root pos
				rootYPos -= (app->fontSectionTitle->getHeight() + 4);
			}
		}

		// maybe battery?
		//TRACE("testing if we we have a battery to show...");
		if (!this->app->skin->showBatteryIcons && !infoBarIsInPlay) {
			// grab the new y offset and write the battery
			//TRACE("we have a battery to show");
			std::string batteryLevel = app->hw->Power()->displayLevel();
			app->screen->write(
				app->fontSectionTitle, 
				batteryLevel, 
				app->sectionBarRect.x + (app->sectionBarRect.w / 2), 
				rootYPos,
				HAlignCenter | VAlignTop);

			// re-calc the y root pos
			rootYPos -= (app->fontSectionTitle->getHeight() + 4);
		}

		// tray helper icons
		//TRACE("hitting up the helpers");
		helpers.push_back(iconVolume[currentVolumeMode]);
		if (this->app->skin->showBatteryIcons) {
			helpers.push_back(iconBattery[batteryIcon]);
		}
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
				if (!app->menu->selLinkApp()->getClock().empty()) {
					// CPU indicator
					helpers.push_back(iconCPU);
				}
			}
		}
		//TRACE("layoutHelperIcons");
		layoutHelperIcons(helpers, rootXPos, rootYPos);
		//TRACE("helpers.clear()");
		helpers.clear();

	}

    //TRACE("flip"); 
	app->screen->flip();
	this->locked_ = false;
    TRACE("exit");
}

void Renderer::layoutHelperIcons(std::vector<Surface*> icons, int rootXPos, int rootYPos) {
	TRACE("enter");

	int helperHeight = 20;
	int iconsPerRow = 0;
	if (app->sectionBarRect.w > app->sectionBarRect.h) {
		iconsPerRow = (int)(app->sectionBarRect.h / (float)helperHeight);
		//TRACE("horizontal mode - %i items in %i pixels", iconsPerRow, app->sectionBarRect.h);
	} else {
		iconsPerRow = (int)(app->sectionBarRect.w / (float)helperHeight);
		//TRACE("vertical mode - %i items in %i pixels", iconsPerRow, app->sectionBarRect.w);
	}

	if (0 == iconsPerRow) {
		WARNING("Section bar is too small for icon size, skipping");
		return;
	}

	int iconCounter = 1;
	int currentXOffset = 0;
	int currentYOffset = 0;

	for(std::vector<Surface*>::iterator it = icons.begin(); it != icons.end(); ++it) {
		//TRACE("blitting");
		Surface *surface = (*it);
		if (NULL == surface)
			continue;
		surface->blit(
			app->screen, 
			rootXPos - (currentXOffset * (helperHeight - 2)), 
			rootYPos - (currentYOffset * (helperHeight - 2))
		);
		if (app->sectionBarRect.w > app->sectionBarRect.h) {
			if (iconCounter % iconsPerRow == 0) {
				++currentXOffset;
				currentYOffset = 0;
			} else {
				++currentYOffset;
			}
		} else {
			if (iconCounter % iconsPerRow == 0) {
				++currentYOffset;
				currentXOffset = 0;
			} else {
				++currentXOffset;
			}
		}
		iconCounter++;
	};
	TRACE("exit");
}

void Renderer::pollHW() {
	TRACE("enter");

	// save battery life
	if (this->app->screenManager->isAsleep())
		return;

	this->polling_ = true;

		try {
		//TRACE("section bar test");
		if (this->app->skin->sectionBar) {
			//TRACE("section bar exists in skin settings");
			//TRACE("updating helper icon status");
			this->app->hw->Power()->read();
			if (this->app->skin->showBatteryIcons) {
				if (IPower::PowerStates::CHARGING == this->app->hw->Power()->state()) {
					TRACE("battery is charging");
					this->batteryIcon = 6;
				} else {
					int level = this->app->hw->Power()->rawLevel();
					TRACE("got raw battery level : %i", level);
					int scale = 6;
					TRACE("battery scaling factor : %i", scale);
					this->batteryIcon = ((scale / (float)100) * level) - 1;
					TRACE("unbounded battery level : %i", this->batteryIcon);
					this->batteryIcon = constrain(this->batteryIcon, 0, scale - 1);
					TRACE("final battery level : %i", this->batteryIcon);
				}
			}

			this->brightnessIcon = this->app->hw->getBacklightLevel();
			if (this->brightnessIcon > 4 || this->iconBrightness[this->brightnessIcon] == NULL) 
				this->brightnessIcon = 5;

			int currentVolume = this->app->hw->Soundcard()->getVolume();
			this->currentVolumeMode = this->getVolumeMode(currentVolume);

			//TRACE("helper icon status updated");
		}

		//TRACE("checking clock skin flag");
		if (this->app->skin->showClock) {
			TRACE("refreshing the clock");
			app->hw->Clock()->getDateTime();
		}
	} catch(std::exception e) {
		ERROR("caught : '%s'", e.what());
	} catch (...) {
		ERROR("unknown error");
	}

	this->app->inputManager->noop();
	this->polling_ = false;
	TRACE("exit");

}

uint32_t Renderer::callback(uint32_t interval, void * data) {
	TRACE("enter");
	Renderer * me = static_cast<Renderer*>(data);
	if (me->finished_) {
		TRACE("callback returning early because of finished flag");
		return 0;
	}
	me->pollHW();
	TRACE("exit : %i", interval);
	return interval;
}

uint8_t Renderer::getVolumeMode(uint8_t vol) {
	TRACE("getVolumeMode - enter : %i", vol);
	if (!vol) return VOLUME_MODE_MUTE;
	else if (vol > 0 && vol < 20) return VOLUME_MODE_PHONES;
	return VOLUME_MODE_NORMAL;
}