/**
 * @file
 */

#include "command/Command.h"
#include "KeybindingHandler.h"
#include "CustomButtonNames.h"
#include "app/App.h"
#include "core/ArrayLength.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Log.h"
#include "io/Filesystem.h"
#include <SDL.h>

namespace util {

static inline bool checkModifierBitMask(int16_t mask, int16_t pressedModMask, int16_t commandModMask) {
	// extract that stuff we are interested in
	const int16_t command = commandModMask & mask;
	const int16_t pressed = pressedModMask & mask;
	if (pressed == mask) {
		// both of them ... erm.. no
		return false;
	}
	if (command == mask) {
		// command is bound to left or right modifier key
		if (!(pressed & mask)) {
			// but the modifiers aren't pressed
			return false;
		}
	} else if (command != pressed) {
		// only one of the modifiers is bound - so values must be equal in order to match
		return false;
	}
	// looks like the values are matching up
	return true;
}

bool isValidForBinding(int16_t pressedModMask, int16_t commandModMask) {
	if (commandModMask == KMOD_NONE && pressedModMask != KMOD_NONE) {
		return false;
	}
	if (commandModMask != KMOD_NONE) {
		if (!checkModifierBitMask(KMOD_SHIFT, pressedModMask, commandModMask)) {
			return false;
		}
		if (!checkModifierBitMask(KMOD_ALT, pressedModMask, commandModMask)) {
			return false;
		}
		if (!checkModifierBitMask(KMOD_CTRL, pressedModMask, commandModMask)) {
			return false;
		}
	}
	return true;
}

/**
 * @param bindings Map of bindings
 * @param key The key that was pressed
 * @param modMask The modifier mask
 * @return @c true if the key+modifier combination lead to a command execution via
 * key bindings, @c false otherwise
 */
static bool executeCommandsForBinding(const BindMap& bindings, int32_t key, int16_t modMask, double nowSeconds, uint16_t count) {
	auto range = bindings.equal_range(key);
	const int16_t modifier = modMask & (KMOD_SHIFT | KMOD_CTRL | KMOD_ALT);
	bool handled = false;
	for (auto i = range.first; i != range.second; ++i) {
		if (count > 0 && i->second.count != count) {
			continue;
		}
		const core::String& command = i->second.command;
		const int16_t mod = i->second.modifier;
		if (!isValidForBinding(modifier, mod)) {
			continue;
		}
		Log::trace("Execute the command %s for key %i", command.c_str(), key);
		if (command[0] == COMMAND_PRESSED[0]) {
			if (command::Command::execute("%s %i %f", command.c_str(), key, nowSeconds) == 1) {
				Log::trace("The tracking command was executed");
				handled = true;
				continue;
			}
			Log::trace("Failed to execute the tracking command %s", command.c_str());
			continue;
		}
		handled |= command::Command::execute(command) > 0;
	}
	return handled;
}

bool KeyBindingHandler::executeCommands(int32_t key, int16_t modifier, double nowSeconds, uint16_t count) {
	// first try to find an exact match of key and current held modifiers
	if (executeCommandsForBinding(_bindings, key, modifier, nowSeconds, count)) {
		return true;
	}
	// if no such exact match was found, try to remove those modifiers that should be ignored because they e.g. have their own bound command
	// this might happen if you bound a command to e.g. shift. Having shift pressed while pressing another key combination like ctrl+w would
	// not match if that special bound key shift wouldn't get removed from the mask to check.
	if (_pressedModifierMask != 0u && executeCommandsForBinding(_bindings, key, (int16_t)((uint32_t)modifier ^ _pressedModifierMask), nowSeconds, count)) {
		return true;
	}
	// at last try to find a key that was bound without any modifier.
	if (executeCommandsForBinding(_bindings, key, 0, nowSeconds, count)) {
		return true;
	}
	return false;
}

void KeyBindingHandler::construct() {
	command::Command::registerCommand("bindlist", [this] (const command::CmdArgs& args) {
		for (BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
			const CommandModifierPair& pair = i->second;
			const core::String& command = pair.command;
			const core::String& keyBinding = getKeyBindingsString(command.c_str(), pair.count);
			Log::info("%-25s %s", keyBinding.c_str(), command.c_str());
		}
	}).setHelp("Show all known key bindings");

	command::Command::registerCommand("bind", [this] (const command::CmdArgs& args) {
		if (args.size() != 2) {
			Log::error("Expected parameters: key+modifier command - got %i parameters", (int)args.size());
			return;
		}

		KeybindingParser p(args[0], args[1]);
		const BindMap& bindings = p.getBindings();
		for (BindMap::const_iterator i = bindings.begin(); i != bindings.end(); ++i) {
			const uint32_t key = i->first;
			const CommandModifierPair& pair = i->second;
			auto range = _bindings.equal_range(key);
			bool found = false;
			for (auto it = range.first; it != range.second; ++it) {
				if (it->second.modifier == pair.modifier) {
					it->second.command = pair.command;
					found = true;
					Log::info("Updated binding for key %s", args[0].c_str());
					break;
				}
			}
			if (!found) {
				_bindings.insert(std::make_pair(key, pair));
				Log::info("Added binding for key %s", args[0].c_str());
			}
		}
	}).setHelp("Bind a command to a key");
}

void KeyBindingHandler::shutdown() {
	core::String keybindings;
	for (BindMap::const_iterator i = _bindings.begin(); i != _bindings.end(); ++i) {
		const int32_t key = i->first;
		const CommandModifierPair& pair = i->second;
		const int16_t modifier = pair.modifier;
		const core::String& command = pair.command;
		keybindings += toString(key, modifier);
		keybindings += " \"";
		keybindings += command;
		keybindings += "\"\n";
	}
	Log::trace("%s", keybindings.c_str());
	io::filesystem()->write("keybindings.cfg", keybindings);
}

bool KeyBindingHandler::init() {
	return true;
}

bool KeyBindingHandler::load(const core::String& filename) {
	const core::String& bindings = io::filesystem()->load(filename);
	if (bindings.empty()) {
		return false;
	}
	Log::debug("Load key bindings from %s", filename.c_str());
	const KeybindingParser p(bindings);
	for (const auto& entry : p.getBindings()) {
		auto i = _bindings.find(entry.first);
		// don't add the same binding more than once
		if (i != _bindings.end() && i->second.command == entry.second.command && i->second.modifier == entry.second.modifier) {
			continue;
		}
		_bindings.insert(std::make_pair(entry.first, entry.second));
	}
	return true;
}

void KeyBindingHandler::setBindings(const BindMap& bindings) {
	_bindings = bindings;
}

core::String KeyBindingHandler::toString(int32_t key, int16_t modifier, uint16_t count) {
	const core::String& name = getKeyName(key, count);
	if (modifier <= 0) {
		return name;
	}
	const char *modifierName = getModifierName(modifier);
	core_assert(modifierName != nullptr);
	return core::string::format("%s+%s", modifierName, name.c_str());
}

core::String KeyBindingHandler::getKeyBindingsString(const char *cmd, uint16_t count) const {
	int16_t modifier;
	int32_t key;
	if (!resolveKeyBindings(cmd, &modifier, &key, nullptr)) {
		return "";
	}
	return toString(key, modifier, count);
}

bool KeyBindingHandler::resolveKeyBindings(const char *cmd, int16_t* modifier, int32_t* key, uint16_t *count) const {
	const char *match = SDL_strchr(cmd, ' ');
	const size_t size = match != nullptr ? (size_t)(intptr_t)(match - cmd) : SDL_strlen(cmd);
	for (const auto& b : _bindings) {
		const CommandModifierPair& pair = b.second;
		if (!SDL_strcmp(pair.command.c_str(), cmd) || !SDL_strncmp(pair.command.c_str(), cmd, size)) {
			if (modifier != nullptr) {
				*modifier = pair.modifier;
			}
			if (count != nullptr) {
				*count = pair.count;
			}
			if (key != nullptr) {
				*key = b.first;
			}
			return true;
		}
	}
	return false;
}

core::String KeyBindingHandler::getKeyName(int32_t key, uint16_t count) {
	for (int i = 0; i < lengthof(button::CUSTOMBUTTONMAPPING); ++i) {
		if (button::CUSTOMBUTTONMAPPING[i].key == key && button::CUSTOMBUTTONMAPPING[i].count == count) {
			return button::CUSTOMBUTTONMAPPING[i].name;
		}
	}
	const char*keyBinding = SDL_GetKeyName((SDL_Keycode)key);
	const core::String lower(keyBinding);
	core::String l = lower.toLower();
	core::string::replaceAllChars(l, ' ', '_');
	return l;
}

const char* KeyBindingHandler::getModifierName(int16_t modifier) {
	if (modifier == 0) {
		return nullptr;
	}
	for (int i = 0; i < lengthof(button::MODIFIERMAPPING); ++i) {
		if (button::MODIFIERMAPPING[i].modifier == modifier) {
			return button::MODIFIERMAPPING[i].name;
		}
	}
	return "<unknown>";
}

bool KeyBindingHandler::execute(int32_t key, int16_t modifier, bool pressed, double nowSeconds, uint16_t count) {
	int16_t code = 0;
	switch (key) {
	case SDLK_LCTRL:
		code = KMOD_LCTRL;
		break;
	case SDLK_RCTRL:
		code = KMOD_RCTRL;
		break;
	case SDLK_LSHIFT:
		code = KMOD_LSHIFT;
		break;
	case SDLK_RSHIFT:
		code = KMOD_RSHIFT;
		break;
	case SDLK_LALT:
		code = KMOD_LALT;
		break;
	case SDLK_RALT:
		code = KMOD_RALT;
		break;
	}

	if (pressed) {
		_keys.insert(key);

		if (code != 0) {
			// this is the case where a binding that needs a modifier should get executed, but
			// a key was pressed in the order: "key and then modifier".
			core::Set<int32_t> recheck;
			for (auto& b : _bindings) {
				const CommandModifierPair& pair = b.second;
				if (pair.count != count) {
					continue;
				}
				const int32_t commandKey = b.first;
				if (pair.command[0] != COMMAND_PRESSED[0]) {
					// no action button command
					continue;
				}
				if (pair.modifier == 0) {
					continue;
				}
				if (!isPressed(commandKey)) {
					continue;
				}
				if (!isValidForBinding(modifier, pair.modifier)) {
					continue;
				}
				command::Command::execute("%s %i %f", pair.command.c_str(), commandKey, nowSeconds);
				recheck.insert(commandKey);
			}
			// for those keys that were activated because only a modifier was pressed (bound to e.g. left_shift),
			// we have to disable the old action button that was just bound to the key without the modifier.
			for (auto checkKey : recheck) {
				auto range = _bindings.equal_range(checkKey->key);
				for (auto i = range.first; i != range.second; ++i) {
					const CommandModifierPair& pair = i->second;
					if (pair.modifier != 0) {
						continue;
					}
					command::Command::execute(COMMAND_RELEASED "%s %i %f", &(pair.command.c_str()[1]), checkKey->key, nowSeconds);
				}
			}
		}
		const bool retVal = executeCommands(key, modifier, nowSeconds, count);
		if (retVal) {
			_pressedModifierMask |= (uint32_t)code;
		}
		return retVal;
	}
	bool handled = false;
	if (code != 0) {
		for (auto& b : _bindings) {
			const CommandModifierPair& pair = b.second;
			const int32_t commandKey = b.first;
			if (pair.command[0] != COMMAND_PRESSED[0]) {
				// no action button command
				continue;
			}
			if (!isValidForBinding(code, pair.modifier)) {
				continue;
			}
			if (!isPressed(commandKey)) {
				continue;
			}
			command::Command::execute(COMMAND_RELEASED "%s %i %f", &(pair.command.c_str()[1]), commandKey, nowSeconds);
			executeCommands(commandKey, modifier, nowSeconds, 0u);
		}
		_pressedModifierMask &= ~(uint32_t)code;
	}
	auto range = _bindings.equal_range(key);
	for (auto i = range.first; i != range.second; ++i) {
		const CommandModifierPair& pair = i->second;
		const core::String& command = pair.command;
		if (command[0] == COMMAND_PRESSED[0]) {
			handled = command::Command::execute(COMMAND_RELEASED "%s %i %f", &(command.c_str()[1]), key, nowSeconds) > 0;
		}
	}
	_keys.remove(key);
	return handled;
}

}
