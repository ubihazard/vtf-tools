#include "vtfshared.hxx"

struct vtf_flag_mod {
	unsigned pattern;
	unsigned rem;
	unsigned add;
	unsigned inv;
};

struct vtf_flag_mod* mods;
unsigned mods_num;

int to_lcase (int const c)
{
	if (c >= 'a' && c <= 'z') {
		return c | 0x20;
	}
	return c;
}

const char* strcasestr (const char* const where
, const char* const what)
{
	auto in = where;
	auto wh = what;
	while (true) {
		while (to_lcase(in[0]) != to_lcase(wh[0])) {
			if (in[0] == 0) {
				return NULL;
			}
			++in;
		}
		do {
			if (wh[0] == 0) {
				return (wh == what) ? NULL : in;
			}
			if (in[0] == 0) {
				return NULL;
			}
			++in;
			++wh;
		} while (to_lcase(in[0]) == to_lcase(wh[0]));
		wh = what;
	}
}

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Expecting a single input
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc != 1 && argc < 3) {
		printf("Error: a texture, a flag pattern, and at least one modifier must be specified\n");
usage:
		printf("Usage: %s input.vtf <match pattern: 0xffffffff> <mod: +0x2|-0x2|/0x2 (add, remove, invert)>\n", app);
		printf("VTF flags reference: https://developer.valvesoftware.com/wiki/Valve_Texture_Format\n");
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the texture
	auto vtf = CVTFFile{};
	auto const fname = argv[0];
	if (!vtf.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}
	auto const flags = vtf.GetFlags();

	if (argc == 1) {
		// Print the header
		auto c = strrchr(fname, '\\');
		if (c == NULL) c = strrchr(fname, '/');
		if (c == NULL) return EXIT_FAILURE;
		printf("%s\n", ++c);
		auto const len = strlen(c);
		for (auto i = 0; i < len; ++i) putchar('-');
		putchar('\n');

		// Print out the texture flags
		printf("Point Sampling:       [%c]\n", bool(flags & 0x0001) ? 'x' : ' ');
		printf("Trilinear Sampling:   [%c]\n", bool(flags & 0x0002) ? 'x' : ' ');
		printf("Clamp S:              [%c]\n", bool(flags & 0x0004) ? 'x' : ' ');
		printf("Clamp T:              [%c]\n", bool(flags & 0x0008) ? 'x' : ' ');
		printf("Anisotropic Sampling: [%c]\n", bool(flags & 0x0010) ? 'x' : ' ');
		printf("Hint DXT5 (Skybox):   [%c]\n", bool(flags & 0x0020) ? 'x' : ' ');
		//printf("PWL Corrected:        [%c]\n", bool(flags & 0x0040) ? 'x' : ' ');
		printf("SRGB (No Compress):   [%c]\n", bool(flags & 0x0040) ? 'x' : ' ');
		printf("Normal Map:           [%c]\n", bool(flags & 0x0080) ? 'x' : ' ');
		printf("No Mipmaps:           [%c]\n", bool(flags & 0x0100) ? 'x' : ' ');
		printf("No Level Of Detail:   [%c]\n", bool(flags & 0x0200) ? 'x' : ' ');
		printf("No Minimum Mipmap:    [%c]\n", bool(flags & 0x0400) ? 'x' : ' ');
		printf("Procedural:           [%c]\n", bool(flags & 0x0800) ? 'x' : ' ');
		printf("One Bit Alpha:        [%c]\n", bool(flags & 0x1000) ? 'x' : ' ');
		printf("Eight Bit Alpha:      [%c]\n", bool(flags & 0x2000) ? 'x' : ' ');
		printf("Environment Map:      [%c]\n", bool(flags & 0x4000) ? 'x' : ' ');
		printf("Render Target:        [%c]\n", bool(flags & 0x8000) ? 'x' : ' ');
		printf("Depth Render Target:  [%c]\n", bool(flags & 0x10000) ? 'x' : ' ');
		printf("No Debug Override:    [%c]\n", bool(flags & 0x20000) ? 'x' : ' ');
		printf("Single Copy:          [%c]\n", bool(flags & 0x40000) ? 'x' : ' ');
		printf("Pre SRGB:             [%c]\n", bool(flags & 0x80000) ? 'x' : ' ');
		//printf("One Over Mipmap:      [%c]\n", bool(flags & 0x80000) ? 'x' : ' ');
		printf("Premultiply Color:    [%c]\n", bool(flags & 0x100000) ? 'x' : ' ');
		printf("Normal To DuDv:       [%c]\n", bool(flags & 0x200000) ? 'x' : ' ');
		printf("Alpha Test Mipmap:    [%c]\n", bool(flags & 0x400000) ? 'x' : ' ');
		printf("No Depth Buffer:      [%c]\n", bool(flags & 0x800000) ? 'x' : ' ');
		printf("Nice Filtered:        [%c]\n", bool(flags & 0x1000000) ? 'x' : ' ');
		printf("Clamp U:              [%c]\n", bool(flags & 0x2000000) ? 'x' : ' ');
		printf("Vertex Texture:       [%c]\n", bool(flags & 0x4000000) ? 'x' : ' ');
		printf("SSBump:               [%c]\n", bool(flags & 0x8000000) ? 'x' : ' ');
		printf("Border:               [%c]\n", bool(flags & 0x20000000) ? 'x' : ' ');

		return EXIT_SUCCESS;
	}

	// Get the flag mods
	++argv, --argc;
	while (argc--) {
		unsigned val;
		auto const mod_rem = argv[0][0] == '-';
		auto const mod_add = argv[0][0] == '+';
		auto const mod_inv = argv[0][0] == '/';
		auto is_mod = mod_rem | mod_add | mod_inv;

		if (is_mod) {
			if (strcasestr (argv[0], ".vtf")) {
				is_mod = false;
			}
		}
		if (is_mod) {
			if (mods_num == 0) {
				printf("A flags modifier must be preceded by a flag pattern\n");
				goto usage;
			}

			val = unsigned(strtoul(argv[0] + 1, NULL, 16));
			if (val == 0/* && errno == EINVAL*/) {
				printf("Invalid flag value: %x\n", val);
				goto usage;
			}

			if (mod_rem) mods[mods_num - 1].rem = val;
			else if (mod_add) mods[mods_num - 1].add = val;
			else mods[mods_num - 1].inv = val;
		} else {
			val = unsigned(strtoul(argv[0], NULL, 16));
			if (val == 0/* && errno == EINVAL*/) {
				printf("Invalid pattern: %x\n", val);
				goto usage;
			}

			mods = re_cast<struct vtf_flag_mod*>(realloc (mods, ++mods_num * sizeof (mods[0])));
			mods[mods_num - 1] = {val, 0, 0, 0};
		}

		++argv;
	}

	// Generate the new flags
	auto flags_new = flags;
	for (auto mods_iter = 0; mods_iter < mods_num; ++mods_iter) {
		if (flags & mods[mods_iter].pattern) {
			flags_new &= ~mods[mods_iter].rem;
			flags_new |= mods[mods_iter].add;
			flags_new ^= mods[mods_iter].inv;
		}
	}

	// Calculate the new VTF size
	auto vtf_size = vtf_get_real_size(vtf);
	auto const thumb_size = vtf.GetThumbnailDataSize();
	auto const img_size = vtf.GetImageDataSize();
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size;

	// Copy the VTF
	auto const vtf_new = malloc(max(vtf_size, vtf_size_new));
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_new, vtf_size, vtf_size);

	// Resave with modified flags
	vtf_force_72(vtf, vtf_new, true);
	*(unsigned*)((char*)vtf_new + VTF_HDR_OFF_FLAGS) = flags_new;

	// Write the file
	if (flags_new != flags) {
		auto const f = fopen(fname, "wb");
		if (f == NULL) {
			printf("Error: couldn't write \"%s\"\n", argv[0]);
			goto fail;
		}
		fwrite(vtf_new, 1, vtf_size_new, f);
		fclose(f);
	}

	// Cleanup
	free(mods);
	free(vtf_new);

	return EXIT_SUCCESS;

fail:
	free(mods);
	free(vtf_new);

	return EXIT_FAILURE;
}
