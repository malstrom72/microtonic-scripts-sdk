// Pick New Patches
//
// GUI-less Microtonic JavaScript example converted from the legacy
// "Pick New Patches.pika" script.
//
// Replaces the drum patches for all unmuted drum channels with randomly picked
// factory patches from the same patch-name category. The script preserves each
// channel's Output and Choke settings, matching the legacy script behavior.

if (!PickNewPatches) {
	var PickNewPatches = {
		inited: false,
		drums: [],
		drumsByCategory: {}
	};
}

(function () {
	var drumDir = DIRS.DRUM_PATCHES + "All/";

	function trimExtension(name) {
		return name.replace(/\.mtdrum$/i, "");
	}

	function categoryToken(token) {
		return token && token.length >= 2 && token.length <= 8;
	}

	function splitTokens(name) {
		var base = trimExtension(name);
		var tokens = base.match(/\S+/g);
		return tokens || [];
	}

	function categoryFromFactoryFile(name) {
		var tokens = splitTokens(name);

		// Legacy factory filenames are expected to look like:
		// "XX Category Patch Name.mtdrum", where XX is an initials/vendor token.
		if (tokens.length >= 3 && tokens[0].length >= 2 && tokens[0].length <= 4 && categoryToken(tokens[1])) {
			return tokens[1].toUpperCase();
		}

		return null;
	}

	function categoriesFromPatchName(name) {
		var tokens = splitTokens(name);
		var categories = [];

		// "Category Patch Name"
		if (tokens.length >= 2 && categoryToken(tokens[0])) {
			categories[categories.length] = tokens[0].toUpperCase();
		}

		// "XX Category Patch Name"
		if (tokens.length >= 3 && tokens[0].length >= 2 && tokens[0].length <= 4 && categoryToken(tokens[1])) {
			categories[categories.length] = tokens[1].toUpperCase();
		}

		return categories;
	}

	function initPickNewPatches() {
		var files;
		var i;
		var file;
		var category;

		if (PickNewPatches.inited) {
			return;
		}

		PickNewPatches.drums = [];
		PickNewPatches.drumsByCategory = {};

		files = dir(drumDir, "mtdrum");
		for (i = 0; i < files.length; ++i) {
			file = files[i];
			if (!file.isDirectory && /\.mtdrum$/i.test(file.name)) {
				PickNewPatches.drums[PickNewPatches.drums.length] = file.name;

				category = categoryFromFactoryFile(file.name);
				if (category) {
					if (!PickNewPatches.drumsByCategory[category]) {
						PickNewPatches.drumsByCategory[category] = [];
					}
					PickNewPatches.drumsByCategory[category][PickNewPatches.drumsByCategory[category].length] = file.name;
				}
			}
		}

		PickNewPatches.inited = true;
	}

	function pickPatchForCategory(category, originalPatch) {
		var choices = PickNewPatches.drumsByCategory[category];
		var name;
		var path;
		var text;
		var replacement;
		var originalOutput;
		var originalChoke;

		if (!choices || choices.length === 0) {
			return null;
		}

		name = choices[random.integer(choices.length)];
		path = drumDir + name;
		text = load(path);

		if (!isMarshaledFormat("drumPatch", text)) {
			return null;
		}

		originalOutput = originalPatch.Output;
		originalChoke = originalPatch.Choke;

		replacement = unmarshal("drumPatch", text);
		replacement.name = trimExtension(name);
		replacement.path = path;
		replacement.Output = originalOutput;
		replacement.Choke = originalChoke;

		return replacement;
	}

	function pickPatch(originalPatch) {
		var categories = categoriesFromPatchName(originalPatch.name);
		var i;
		var replacement;

		for (i = 0; i < categories.length; ++i) {
			replacement = pickPatchForCategory(categories[i], originalPatch);
			if (replacement) {
				return replacement;
			}
		}

		return null;
	}

	var preset = getElement("preset");
	var pickedCount = 0;
	var ch;
	var replacement;

	initPickNewPatches();

	for (ch = 0; ch < CHANNEL_COUNT; ++ch) {
		if (preset.mutes[ch] < 0.5) {
			replacement = pickPatch(preset.drumPatches[ch]);
			if (replacement) {
				preset.drumPatches[ch] = replacement;
				++pickedCount;
			}
		}
	}

	if (pickedCount === 0) {
		display(
			"Oops. None of the (unmuted) patch names contained a known category prefix (e.g. 'BD', 'SD').\n\n" +
			"Preset left untouched."
		);
	} else {
		preset.modified = true;
		setElement("preset", preset);
	}
}());
