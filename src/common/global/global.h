////////////////////////////////////////////////////////////////////////////////
//            Copyright (C) 2004-2011 by The Allacrost Project
//            Copyright (C) 2012-2016 by Bertram (Valyria Tear)
//                         All Rights Reserved
//
// This code is licensed under the GNU GPL version 2. It is free software
// and you may modify it and/or redistribute it under the terms of this license.
// See http://www.gnu.org/copyleft/gpl.html for details.
////////////////////////////////////////////////////////////////////////////////

/** ****************************************************************************
*** \file    global.h
*** \author  Tyler Olsen, roots@allacrost.org
*** \author  Yohann Ferreira, yohann ferreira orange fr
*** \brief   Header file for the global game manager
***
*** This file contains the GameGlobal class, which is used to manage all data
*** that is shared "globally" by the various game modes. For example, it
*** contains the current characters in the party, the party's inventory, etc.
*** The definition of characters, items, and other related global data are
*** implemented in the other global header files (e.g. global_actors.h). All
*** of these global files share the same vt_global namespace.
*** ***************************************************************************/

#ifndef __GLOBAL_HEADER__
#define __GLOBAL_HEADER__

#include "utils/utils_strings.h"

#include "script/script_read.h"
#include "script/script_write.h"

#include "media/global_media.h"
#include "media/battle_media.h"

#include "skill_graph/skill_graph.h"
#include "actors/global_character_handler.h"
#include "actors/global_actor.h"
#include "actors/global_party.h"

#include "objects/global_inventory_handler.h"
#include "objects/global_armor.h"
#include "objects/global_weapon.h"

#include "global_skills.h"

#include "events/global_events.h"
#include "quests/quests.h"
#include "shop_data.h"
#include "worldmap_location.h"

#include "maps/map_data_handler.h"

//! \brief All calls to global code are wrapped inside this namespace.
namespace vt_global
{

class GameGlobal;
class GlobalSpirit;

//! \brief The singleton pointer responsible for the management of global game data.
extern GameGlobal* GlobalManager;

//! \brief Determines whether the code in the vt_global namespace should print debug statements or not.
extern bool GLOBAL_DEBUG;

/** ****************************************************************************
*** \brief Retains all the state information about the active game
***
*** This class is a resource manager for the current state of the game that is
*** being played.
***
*** \note This class is a singleton, even though it is technically not an engine
*** manager class. There can only be one game instance that the player is playing
*** at any given time.
*** ***************************************************************************/
class GameGlobal : public vt_utils::Singleton<GameGlobal>
{
    friend class vt_utils::Singleton<GameGlobal>;

public:
    ~GameGlobal();

    bool SingletonInitialize();

    //! Reloads the persistent scripts. Used when changing the language for instance.
    bool ReloadGlobalScripts()
    { _CloseGlobalScripts(); return _LoadGlobalScripts(); }

    /** \brief Deletes all data stored within the GameGlobal class object
    *** This function is meant to be called when the user quits the current game instance
    *** and returns to the boot screen. It will delete all characters, inventory, and other
    *** data relevant to the current game.
    **/
    void ClearAllData();

    //! \brief Executes function NewGame() from global script
    //! \returns whether it succeeded.
    bool NewGame();

    /** \brief Loads all global data from a saved game file
    *** \param filename The filename of the saved game file where to read the data from
    *** \param slot_id The save slot the file correspond to. Used to set the correct cursor position
    *** when further saving.
    *** \return True if the game was successfully loaded, false if it was not
    **/
    bool LoadGame(const std::string &filename, uint32_t slot_id);

    /** \brief Saves all global data to a saved game file
    *** \param filename The filename of the saved game file where to write the data to
    *** \param slot_id The game slot id used for the save menu.
    *** \param positions When used in a save point, the save map tile positions are given there.
    *** \return True if the game was successfully saved, false if it was not
    **/
    bool SaveGame(const std::string &filename, uint32_t slot_id, uint32_t x_position = 0, uint32_t y_position = 0);

    //! \brief Attempts an autosave on the current slot, using given map and location.
    bool AutoSave(const std::string& map_data_file, const std::string& map_script_file,
                  uint32_t stamina,
                  uint32_t x_position = 0, uint32_t y_position = 0);

    //! \brief Gets the last load/save position.
    uint32_t GetGameSlotId() const {
        return _game_slot_id;
    }

    //! \note The overflow condition is not checked here: we just assume it will never occur
    void AddDrunes(uint32_t amount) {
        _drunes += amount;
    }

    //! \note The amount is only subtracted if the current funds is equal to or exceeds the amount to subtract
    void SubtractDrunes(uint32_t amount) {
        if(_drunes >= amount) _drunes -= amount;
    }

    void SetDrunes(uint32_t amount) {
        _drunes = amount;
    }

    uint32_t GetDrunes() const {
        return _drunes;
    }

    void SetMaxExperienceLevel(uint32_t level) {
        _max_experience_level = level;
    }

    uint32_t GetMaxExperienceLevel() const {
        return _max_experience_level;
    }

    //! \brief Tells whether an enemy id is existing in the enemy data.
    bool DoesEnemyExist(uint32_t enemy_id);

    //! \brief gets the current world map image
    //! \return a pointer to the currently viewable World Map Image.
    //! \note returns nullptr if the filename has been set to ""
    vt_video::StillImage *GetWorldMapImage() const
    {
        return _world_map_image;
    }

    const std::string &GetWorldMapFilename() const
    {
        if (_world_map_image)
            return _world_map_image->GetFilename();
        else
            return vt_utils::_empty_string;
    }

    /** \brief sets the current viewable world map
    *** empty strings are valid, and will cause the return
    *** of a null pointer on GetWorldMap call.
    *** \note this will also clear the currently viewable locations and the current location id
    **/
    void SetWorldMap(const std::string& world_map_filename)
    {
        if (_world_map_image)
            delete _world_map_image;

        _viewable_world_locations.clear();
        _current_world_location_id.clear();
        _world_map_image = new vt_video::StillImage();
        _world_map_image->Load(world_map_filename);
    }

    /** \brief Sets the current location id
    *** \param the location id of the world location that is defaulted to as "here"
    *** when the world map menu is opened
    **/
    void SetCurrentLocationId(const std::string& location_id)
    {
        _current_world_location_id = location_id;
    }

    /** \brief adds a viewable location string id to the currently viewable
    *** set. This string IDs are maintained in the data/config/world_location.lua file.
    *** \param the string id to the currently viewable location
    **/
    void ShowWorldLocation(const std::string& location_id)
    {
        //defensive check. do not allow blank ids.
        //if you want to remove an id, call HideWorldLocation
        if(location_id.empty())
            return;
        // check to make sure this location isn't already visible
        if(std::find(_viewable_world_locations.begin(),
                     _viewable_world_locations.end(),
                     location_id) == _viewable_world_locations.end())
        {
            _viewable_world_locations.push_back(location_id);
        }
    }

    /** \brief removes a location from the currently viewable list
    *** if the id doesn't exist, we don't do anything
    *** \param the string id to the viewable location we want to hide
    **/
    void HideWorldLocation(const std::string &location_id)
    {
        std::vector<std::string>::iterator rem_iterator = std::find(_viewable_world_locations.begin(),
                                                          _viewable_world_locations.end(),
                                                          location_id);
        if(rem_iterator != _viewable_world_locations.end())
            _viewable_world_locations.erase((rem_iterator));
    }

    /** \brief gets a reference to the current viewable location ids
    *** \return reference to the current viewable location ids
    **/
    const std::vector<std::string>& GetViewableLocationIds() const
    {
        return _viewable_world_locations;
    }

    /** \brief get a pointer to the associated world location for the id
    *** \param string Reference if for the world map location
    *** \return nullptr if the location does not exist. otherwise, return a const pointer
    *** to the location
    **/
    WorldMapLocation* GetWorldLocation(const std::string &id)
    {
        std::map<std::string, WorldMapLocation>::iterator itr = _world_map_locations.find(id);
        return itr == _world_map_locations.end() ? nullptr : &(itr->second);
    }

    /** \brief Gets a reference to the current world location id
    *** \return Reference to the current id. this value always exists, but could be "" if
    *** the location is not set, or if the world map is cleared
    *** the value could also not currently exist, if HideWorldLocation was called on an
    *** id that was also set as the current location. the calling code should check for this
    **/
    const std::string &GetCurrentLocationId() const
    {
        return _current_world_location_id;
    }

    //! \brief Gives the shop data corresponding to the current shop id.
    // Used to sync a given shop or save games
    const ShopData& GetShopData(const std::string& shop_id) {
        if (_shop_data.find(shop_id) == _shop_data.end())
            return _shop_data[std::string()]; // Return default empty shop data
        return _shop_data.at(shop_id);
    }

    bool HasShopData(const std::string& shop_id) const {
        return (_shop_data.find(shop_id) != _shop_data.end());
    }

    //! \brief Sets the current shop data to global manager.
    void SetShopData(const std::string& shop_id, const ShopData& shop_data);

    vt_script::ReadScriptDescriptor &GetWeaponSkillsScript() {
        return _weapon_skills_script;
    }

    vt_script::ReadScriptDescriptor &GetMagicSkillsScript() {
        return _magic_skills_script;
    }

    vt_script::ReadScriptDescriptor &GetSpecialSkillsScript() {
        return _special_skills_script;
    }

    vt_script::ReadScriptDescriptor &GetBareHandsSkillsScript() {
        return _bare_hands_skills_script;
    }

    vt_script::ReadScriptDescriptor &GetStatusEffectsScript() {
        return _status_effects_script;
    }

    vt_script::ReadScriptDescriptor &GetCharactersScript() {
        return _characters_script;
    }

    vt_script::ReadScriptDescriptor &GetEnemiesScript() {
        return _enemies_script;
    }

    vt_script::ReadScriptDescriptor &GetMapSpriteScript() {
        return _map_sprites_script;
    }

    //! \brief loads the emotes used for character feelings expression in the given lua file.
    void LoadEmotes(const std::string &emotes_filename);

    //! \brief Set up the offsets for the given emote animation and sprite direction.
    void GetEmoteOffset(float &x, float &y, const std::string &emote_id, vt_map::private_map::ANIM_DIRECTIONS dir);

    //! \brief Tells whether an emote id exists and is valid
    bool DoesEmoteExist(const std::string& emote_id) {
        return (_emotes.count(emote_id));
    }

    //! \brief Get a pointer reference to the given emote animation. Don't delete it!
    vt_video::AnimatedImage* GetEmoteAnimation(const std::string& emote_id) {
        if(_emotes.find(emote_id) != _emotes.end()) return &_emotes.at(emote_id);
        else return nullptr;
    }

    //! \brief Get a reference to the skill graph handler
    CharacterHandler& GetCharacterHandler() {
        return _character_handler;
    }

    //! \brief Get a reference to inventory handler
    InventoryHandler& GetInventoryHandler() {
        return _inventory_handler;
    }

    //! \brief Get the reference to the skill graph handler
    SkillGraph& GetSkillGraph() {
        return _skill_graph;
    }

    GameEvents& GetGameEvents() {
        return _game_events;
    }

    GameQuests& GetGameQuests() {
        return _game_quests;
    }

    MapDataHandler& GetMapData() {
        return _map_data_handler;
    }

    //! \brief Gives access to global media files.
    //! Note: The reference is passed non const to be able to give modifiable references
    //! and pointers.
    GlobalMedia& Media() {
        return _global_media;
    }

    //! \brief Gives access to global battle media files.
    //! Note: The reference is passed non const to be able to give modifiable references
    //! and pointers.
    BattleMedia& GetBattleMedia() {
        return _battle_media;
    }

private:
    GameGlobal();

    //! \brief The slot id the game was loaded from/saved to, or 0 if none.
    uint32_t _game_slot_id;

    //! \brief The amount of financial resources (drunes) that the party currently has
    uint32_t _drunes;

    /** \brief Set the max level that can be reached by a character
    *** This equals 100 by default, @see Set/GetMaxExperienceLevel()
    **/
    uint32_t _max_experience_level;

    //! \brief A map of the curent shop data.
    //! shop_id, corresponding shop data
    std::map<std::string, ShopData> _shop_data;

    //! \brief The container which stores all of the groups of events that have occured in the game
    GameEvents _game_events;

    CharacterHandler _character_handler;

    InventoryHandler _inventory_handler;

    SkillGraph _skill_graph;

    GameQuests _game_quests;

    MapDataHandler _map_data_handler;

    //! \brief member storing all the common media files.
    GlobalMedia _global_media;

    //! \brief member storing all the common battle media files.
    BattleMedia _battle_media;

    //! \name Global data and function script files
    //@{
    //! \brief Contains character ID definitions and a number of useful functions
    vt_script::ReadScriptDescriptor _global_script;

    //! \brief Contains data and functional definitions for all weapon skills
    vt_script::ReadScriptDescriptor _weapon_skills_script;

    //! \brief Contains data and functional definitions for all magic skills
    vt_script::ReadScriptDescriptor _magic_skills_script;

    //! \brief Contains data and functional definitions for all special skills
    vt_script::ReadScriptDescriptor _special_skills_script;

    //! \brief Contains data and functional definitions for all bare hands skills
    vt_script::ReadScriptDescriptor _bare_hands_skills_script;

    //! \brief Contains functional definitions for all status effects
    vt_script::ReadScriptDescriptor _status_effects_script;

    //! \brief Contains data and functional definitions for characters
    vt_script::ReadScriptDescriptor _characters_script;

    //! \brief Contains data and functional definitions for enemies
    vt_script::ReadScriptDescriptor _enemies_script;

    //! \brief Contains data and functional definitions for sprites seen in game maps
    vt_script::ReadScriptDescriptor _map_sprites_script;

    //! \brief Contains data and functional definitions for map objects seen in game maps
    vt_script::ReadScriptDescriptor _map_objects_script;

    //! \brief Contains data and functional definitions for map treasures seen in game maps
    vt_script::ReadScriptDescriptor _map_treasures_script;
    //@}

    //! \brief The current graphical world map. If the filename is empty,
    //! then we are "hiding" the map
    vt_video::StillImage* _world_map_image;

    //! \brief The current viewable location ids on the current world map image
    //! \note this list is cleared when we call SetWorldMap. It is up to the
    //! script writter to maintain the properties of the map by either
    //!  1) call CopyViewableLocationList()
    //!  2) maintain in some other fashion the list
    std::vector<std::string> _viewable_world_locations;

    /** \brief the container which stores all the available world locations in the game.
    *** the world_location_id acts as the key
    **/
    std::map<std::string, WorldMapLocation> _world_map_locations;

    //! \brief the current world map location id that indicates where the player is
    std::string _current_world_location_id;

    //! \brief A map containing all the emote animations
    std::map<std::string, vt_video::AnimatedImage> _emotes;
    //! \brief The map continaing the four sprite direction offsets (x and y value).
    std::map<std::string, std::vector<std::pair<float, float> > > _emotes_offsets;

    /** \brief saves the world map information. this is called from SaveGame()
    *** \param file Reference to open and valid file for writting the data
    **/
    void _SaveWorldMap(vt_script::WriteScriptDescriptor& file);

    /** \brief saves the shop data information. this is called from SaveGame()
    *** \param file Reference to open and valid file for writting the data
    **/
    void _SaveShopData(vt_script::WriteScriptDescriptor& file);

    /** \brief Load world map and viewable information from the save game
    *** \param file Reference to an open file for reading save game data
    **/
    void _LoadWorldMap(vt_script::ReadScriptDescriptor &file);

    /** \brief Helper function called by LoadGlobalScripts() that (re)loads each world location from the script into the world location entry map
    *** \param file Path to the file to world locations script
    *** \return true if successfully loaded
    **/
    bool _LoadWorldLocationsScript(const std::string& world_locations_filename);

    /** \brief Load shop data from the save game
    *** \param file Reference to an open file for reading save game data
    **/
    void _LoadShopData(vt_script::ReadScriptDescriptor& file);

    //! \brief Loads every persistent scripts, used at the global initialization time.
    bool _LoadGlobalScripts();

    //! \brief Unloads every persistent scripts by closing their files.
    void _CloseGlobalScripts();
};

} // namespace vt_global

#endif // __GLOBAL_HEADER__
