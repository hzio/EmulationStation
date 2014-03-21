#include "GuiScraperStart.h"
#include "GuiScraperMulti.h"
#include "GuiMsgBox.h"

#include "../components/TextComponent.h"
#include "../components/OptionListComponent.h"
#include "../components/SwitchComponent.h"

GuiScraperStart::GuiScraperStart(Window* window) : GuiComponent(window),
	mMenu(window, "SCRAPE NOW")
{
	addChild(&mMenu);

	// add filters (with first one selected)
	mFilters = std::make_shared< OptionListComponent<GameFilterFunc> >(mWindow, "SCRAPE THESE GAMES", false);
	mFilters->add("All Games", 
		[](SystemData*, FileData*) -> bool { return true; }, false);
	mFilters->add("Only missing image", 
		[](SystemData*, FileData* g) -> bool { return g->metadata.get("image").empty(); }, true);
	mMenu.addWithLabel("Filter", mFilters);

	//add systems (all with a platformid specified selected)
	mSystems = std::make_shared< OptionListComponent<SystemData*> >(mWindow, "SCRAPE THESE SYSTEMS", true);
	for(auto it = SystemData::sSystemVector.begin(); it != SystemData::sSystemVector.end(); it++)
		mSystems->add((*it)->getFullName(), *it, (*it)->getPlatformId() != PlatformIds::PLATFORM_UNKNOWN);
	mMenu.addWithLabel("Systems", mSystems);

	mApproveResults = std::make_shared<SwitchComponent>(mWindow);
	mApproveResults->setState(true);
	mMenu.addWithLabel("User decides on conflicts", mApproveResults);

	mMenu.addButton("START", "start scraping", std::bind(&GuiScraperStart::pressedStart, this));
	mMenu.addButton("BACK", "cancel", [&] { delete this; });

	mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x()) / 2, Renderer::getScreenHeight() * 0.15f);
}

void GuiScraperStart::pressedStart()
{
	std::vector<SystemData*> sys = mSystems->getSelectedObjects();
	for(auto it = sys.begin(); it != sys.end(); it++)
	{
		if((*it)->getPlatformId() == PlatformIds::PLATFORM_UNKNOWN)
		{
			mWindow->pushGui(new GuiMsgBox(mWindow, 
				"Warning: some of your selected systems do not have a platform ID set. Results may be even more inaccurate than usual!\nContinue anyway?", 
				"YES", std::bind(&GuiScraperStart::start, this), 
				"NO", nullptr));
			return;
		}
	}

	start();
}

void GuiScraperStart::start()
{
	std::queue<ScraperSearchParams> searches = getSearches(mSystems->getSelectedObjects(), mFilters->getSelected());

	GuiScraperMulti* gsm = new GuiScraperMulti(mWindow, searches, mApproveResults->getState());
	mWindow->pushGui(gsm);
	delete this;
}

std::queue<ScraperSearchParams> GuiScraperStart::getSearches(std::vector<SystemData*> systems, GameFilterFunc selector)
{
	std::queue<ScraperSearchParams> queue;
	for(auto sys = systems.begin(); sys != systems.end(); sys++)
	{
		std::vector<FileData*> games = (*sys)->getRootFolder()->getFilesRecursive(GAME);
		for(auto game = games.begin(); game != games.end(); game++)
		{
			if(selector((*sys), (*game)))
			{
				ScraperSearchParams search;
				search.game = *game;
				search.system = *sys;
				
				queue.push(search);
			}
		}
	}

	return queue;
}

bool GuiScraperStart::input(InputConfig* config, Input input)
{
	bool consumed = GuiComponent::input(config, input);
	if(consumed)
		return true;
	
	if(input.value != 0 && config->isMappedTo("b", input))
	{
		delete this;
		return true;
	}

	return false;
}

std::vector<HelpPrompt> GuiScraperStart::getHelpPrompts()
{
	std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("b", "cancel"));
	return prompts;
}