#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Expecting a destination and a source textures
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc != 2) {
		printf("Error: two textures must be specified (got %d)\n", argc);
usage:
		printf("Usage: %s destination.vtf source.vtf\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the destnation texture
	auto vtf_dst = CVTFFile{};
	if (!vtf_dst.Load(argv[0])) {
		printf("Error: couldn't load destination \"%s\"\n", argv[0]);
		goto usage;
	}

	// Load the source texture with alpha
	auto vtf_src = CVTFFile{};
	if (!vtf_src.Load(argv[1])) {
		printf("Error: couldn't load source \"%s\"\n", argv[1]);
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

	// Destination must be in DXT1 or DXT5 format.
	// In case of DXT5 existing alpha will get replaced.
	if ((format != IMAGE_FORMAT_DXT1 && format != IMAGE_FORMAT_DXT5)
	|| (flags & TEXTUREFLAGS_ONEBITALPHA)) {
		printf("Error: \"%s\" must be in DXT1 or DXT5 format [%d]\n", argv[0], format);
		return EXIT_FAILURE;
	}

	// Texture parameters must match
	if (vtf_src.GetWidth() != width) {
		printf("Error: width mismatch (%u vs %u)\n", width, vtf_src.GetWidth());
		return EXIT_FAILURE;
	}
	if (vtf_src.GetHeight() != height) {
		printf("Error: height mismatch (%u vs %u)\n", height, vtf_src.GetHeight());
		return EXIT_FAILURE;
	}
	if (vtf_src.GetDepth() != depth) {
		printf("Error: depth mismatch (%u vs %u)\n", depth, vtf_src.GetDepth());
		return EXIT_FAILURE;
	}
	if (vtf_src.GetMipmapCount() != mips) {
		printf("Error: mipmap count mismatch (%u vs %u)\n", mips, vtf_src.GetMipmapCount());
		return EXIT_FAILURE;
	}
	if (vtf_src.GetFrameCount() != frames) {
		printf("Error: frame count mismatch (%u vs %u)\n", frames, vtf_src.GetFrameCount());
		return EXIT_FAILURE;
	}

	// Source must be in DXT5 format
	if (vtf_src.GetFormat() != IMAGE_FORMAT_DXT5) {
		printf("Error: \"%s\" is not in DXT5 format [%d]\n", argv[1], vtf_src.GetFormat());
		return EXIT_FAILURE;
	}

	// Must have an 8-bit alpha channel
	if (!(vtf_src.GetFlags() & TEXTUREFLAGS_EIGHTBITALPHA)) {
		printf("Error: no 8-bit alpha channel in \"%s\" [0x%x]\n", argv[1], vtf_src.GetFlags());
		return EXIT_FAILURE;
	}

	// "Deep" textures are not supported
	if (depth != 1) {
		printf("Error: unsupported depth in \"%s\" [%d]\n", argv[0], depth);
		return EXIT_FAILURE;
	}

	// Calculate the new image data size with alpha channel
	auto vtf_size = vtf_get_real_size(vtf_dst);
	auto const thumb_size = vtf_dst.GetThumbnailDataSize();
	auto const img_size = vtf_dst.GetImageDataSize();
	auto off = VTF_HDR_SIZE + thumb_size;
	auto const vtf_size_new = [&] {
		if (format == IMAGE_FORMAT_DXT1) {
			return off + img_size * 2;
		} else {
			return off + img_size;
		}
	}();

	// Copy the destination VTF
	auto const vtf_new = malloc(max(vtf_size, vtf_size_new));
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf_dst.Save(vtf_new, vtf_size, vtf_size);
	vtf_force_72(vtf_dst, vtf_new, true);

	// Create gaps for alpha chennel in DXT1 compressed image data
	if (format == IMAGE_FORMAT_DXT1) {
		auto m = mips;
		while (m--) {
			auto const mip = mips - m;
			auto const mip_size = vtf_dst.ComputeImageSize(max(width >> m, 1)
			, max(height >> m, 1), depth, format);
			auto const blocks = mip_size / 8;
			auto f = frames;
			while (f--) {
				auto const num = frames - f;
				auto data = vtf_dst.GetData(num - 1, 0, 0, m);
				auto b = blocks;
				while (b--) {
					memcpy((char*)vtf_new + off + 8, data, 8);
					data += 8;
					off += 16;
				}
			}
		}

		// Change to DXT5 format
		*(int*)((char*)vtf_new + VTF_HDR_OFF_FORMAT) = IMAGE_FORMAT_DXT5;

		// Reset offset to the start of the image data
		off = VTF_HDR_SIZE + thumb_size;
	}

	// Add the 8-bit alpha flag
	*(unsigned int*)((char*)vtf_new + VTF_HDR_OFF_FLAGS) = flags | TEXTUREFLAGS_EIGHTBITALPHA;

	// Copy alpha from the source VTF
	auto m = mips;
	while (m--) {
		auto const mip = mips - m;
		auto const mip_size = vtf_src.ComputeImageSize(max(width >> m, 1)
		, max(height >> m, 1), depth, IMAGE_FORMAT_DXT5);
		auto const blocks = mip_size / 16;
		auto f = frames;
		while (f--) {
			auto const num = frames - f;
			auto data = vtf_src.GetData(num - 1, 0, 0, m);
			auto b = blocks;
			while (b--) {
				memcpy((char*)vtf_new + off, data, 8);
				data += 16;
				off += 16;
			}
		}
	}

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
