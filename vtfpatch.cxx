#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Expecting a destination and a source textures,
	// source region dimensions, destination position,
	// and an optional parameter
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc < 4) {
		printf("Error: two textures must be specified (got %d)\n", argc);
usage:
		printf("Usage: %s destination.vtf source.vtf <top,left,width,height (source rect)> <top,left (destination coords)> [alpha|both]\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the destnation texture
	auto vtf_dst = CVTFFile{};
	if (!vtf_dst.Load(argv[0])) {
		printf("Error: couldn't load destination \"%s\"\n", argv[0]);
		goto usage;
	}

	// Load the source alpha texture
	auto vtf_src = CVTFFile{};
	if (!vtf_src.Load(argv[1])) {
		printf("Error: couldn't load source \"%s\"\n", argv[1]);
		goto usage;
	}

	// Get the source region rectangle
	auto item = strtok(argv[2], ",");
	if (item == NULL) {return EXIT_FAILURE;}
	auto const rx = atoi(item) / 4;
	item = strtok(NULL, ",");
	if (item == NULL) {return EXIT_FAILURE;}
	auto const ry = atoi(item) / 4;
	item = strtok(NULL, ",");
	if (item == NULL) {return EXIT_FAILURE;}
	auto const rw = (atoi(item) + 3) / 4;
	item = strtok(NULL, ",");
	if (item == NULL) {return EXIT_FAILURE;}
	auto const rh = (atoi(item) + 3) / 4;

	// Get the destination position
	item = strtok(argv[3], ",");
	if (item == NULL) {return EXIT_FAILURE;}
	auto const px = atoi(item) / 4;
	item = strtok(NULL, ",");
	if (item == NULL) {return EXIT_FAILURE;}
	auto const py = atoi(item) / 4;

	// What channels to copy
	auto alpha = FALSE;
	auto block_size = 8u;
	auto block_off = 8u;

	if (argc == 5) {
		     if (strieq(argv[4], "alpha")) {alpha = TRUE; block_off = 16u;}
		else if (strieq(argv[4], "both"))  {alpha = TRUE; block_off = block_size = 16u;}
		else {
usage:
			printf("Usage: %s dst.vtf src.vtf dx,dy,dw,dh sx,sy [alpha | both]\n", app);
			return EXIT_FAILURE;
		}
	} else if (argc > 5) {
		goto usage;
	}

	// Get the texture parameters
	auto const width = vtf_dst.GetWidth();
	auto const height = vtf_dst.GetHeight();
	auto const depth = vtf_dst.GetDepth();
	auto const mips = vtf_dst.GetMipmapCount();
	auto const frames = vtf_dst.GetFrameCount();
	auto const format = vtf_dst.GetFormat();
	auto const flags = vtf_dst.GetFlags();

	// Texture parameters must match
	if (vtf_src.GetDepth() != depth) {
		printf("Error: depth mismatch (%u vs %u)\n", depth, vtf_src.GetDepth());
		return EXIT_FAILURE;
	}

	// Get the number of DXT blocks
	auto const wblocks = width / 4;
	auto const hblocks = height / 4;

	if (wblocks == 0 || hblocks == 0) {
		printf("Error: a single texture dimension cannot be less than 4 (%dx%d)\n", width, height);
		return EXIT_FAILURE;
	}

	if (vtf_src.GetWidth() / 4 == 0 || vtf_src.GetHeight() / 4 == 0) {
		printf("Error: a single texture dimension cannot be less than 4 (%dx%d)\n", vtf_src.GetWidth(), vtf_src.GetHeight());
		return EXIT_FAILURE;
	}

	if (alpha) {
		// Destination must be in DXT5
		if (format != IMAGE_FORMAT_DXT5 || !(flags & TEXTUREFLAGS_EIGHTBITALPHA)) {
			printf("Error: \"%s\" must be in DXT5 format [%d]\n", argv[0], format);
			return EXIT_FAILURE;
		}
		// Source must be in DXT5
		if (vtf_src.GetFormat() != IMAGE_FORMAT_DXT5 || !(vtf_src.GetFlags() & TEXTUREFLAGS_EIGHTBITALPHA)) {
			printf("Error: \"%s\" must be in DXT5 format [%d]\n", argv[1], vtf_src.GetFormat());
			return EXIT_FAILURE;
		}
	} else {
		// Destination must be in DXT1 or DXT5 format
		if ((format != IMAGE_FORMAT_DXT1 && format != IMAGE_FORMAT_DXT5)
		|| (flags & TEXTUREFLAGS_ONEBITALPHA)) {
			printf("Error: \"%s\" must be in DXT1 or DXT5 format [%d]\n", argv[0], format);
			return EXIT_FAILURE;
		}
		// Source must be in DXT1 or DXT5 format
		if ((vtf_src.GetFormat() != IMAGE_FORMAT_DXT1 && vtf_src.GetFormat() != IMAGE_FORMAT_DXT5)
		|| (vtf_src.GetFlags() & TEXTUREFLAGS_ONEBITALPHA)) {
			printf("Error: \"%s\" must be in DXT1 or DXT5 format [%d]\n", argv[1], vtf_src.GetFormat());
			return EXIT_FAILURE;
		}
	}

	// "Deep" textures are not supported
	if (depth != 1) {
		printf("Error: unsupported depth in \"%s\" [%d]\n", argv[0], depth);
		return EXIT_FAILURE;
	}

	// Get the number of bytes to move
	auto const bdiv_dst = (format == IMAGE_FORMAT_DXT5) ? 16 : 8;
	auto const bdiv_src = (vtf_src.GetFormat() == IMAGE_FORMAT_DXT5) ? 16 : 8;

	// Calculate the new image data size with alpha channel
	auto vtf_size = vtf_get_real_size(vtf_dst);
	auto const thumb_size = vtf_dst.GetThumbnailDataSize();
	auto const img_size = vtf_dst.GetImageDataSize();
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size;

	// Copy color data at the specified region from the source VTF
	// to the specified position in the destination VTF
	auto data_dst = vtf_dst.GetData(0, 0, 0, 0) + py * wblocks * bdiv_dst + px * bdiv_dst;
	auto data_src = vtf_src.GetData(0, 0, 0, 0) + ry * (vtf_src.GetWidth() / 4) * bdiv_src + rx * bdiv_src;
	auto i = rh;
	while (i--) {
		auto j = rw;
		while (j--) {
			memcpy(data_dst + bdiv_dst - block_off, data_src + bdiv_src - block_off, block_size);
			data_src += bdiv_src;
			data_dst += bdiv_dst;
		}
		data_dst += (wblocks - rw) * bdiv_dst;
		data_src += (vtf_src.GetWidth() / 4 - rw) * bdiv_src;
	}

	// Copy the destination VTF
	auto const vtf_new = malloc(max(vtf_size, vtf_size_new));
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf_dst.Save(vtf_new, vtf_size, vtf_size);

	// Drop the resource block
	vtf_force_72(vtf_dst, vtf_new, true);

	// Write the file
	auto const f = fopen(argv[0], "wb");
	if (f == NULL) {
		printf("Error: couldn't write \"%s\"\n", argv[0]);
		goto fail;
	}
	fwrite(vtf_new, 1, vtf_size_new, f);
	fclose(f);

	// Cleanup
	free(vtf_new);

	return EXIT_SUCCESS;

fail:
	free(vtf_new);

	return EXIT_FAILURE;
}
