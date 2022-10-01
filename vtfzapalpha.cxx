#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Expecting a single input
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc != 1) {
		printf("Error: only one texture must be specified (got %d)\n", argc);
usage:
		printf("Usage: %s input.vtf\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the texture
	auto vtf = CVTFFile{};
	if (!vtf.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}

	// Get the texture parameters
	auto const width = vtf.GetWidth();
	auto const height = vtf.GetHeight();
	auto const depth = vtf.GetDepth();
	auto const mips = vtf.GetMipmapCount();
	auto const frames = vtf.GetFrameCount();
	auto const format = vtf.GetFormat();
	auto const flags = vtf.GetFlags();

	// Must be in DXT5 format
	if (format != IMAGE_FORMAT_DXT5) {
		printf("Error: \"%s\" is not in DXT5 format [%d]\n", argv[0], format);
		return EXIT_FAILURE;
	}

	// Must have an 8-bit alpha channel
	if (!(flags & TEXTUREFLAGS_EIGHTBITALPHA)) {
		printf("Error: no 8-bit alpha channel in \"%s\" [0x%x]\n", argv[0], flags);
		return EXIT_FAILURE;
	}

	// "Deep" textures are not supported
	if (depth != 1) {
		printf("Error: unsupported depth in \"%s\" [%d]\n", argv[0], depth);
		return EXIT_FAILURE;
	}

	// Calculate the new image data size without alpha channel
	auto const thumb_size = vtf.GetThumbnailDataSize();
	auto const img_size = vtf.GetImageDataSize();
	auto off = VTF_HDR_SIZE + thumb_size;
	auto const vtf_size_new = off + img_size / 2;

	// Copy the VTF
	auto vtf_size = vtf_get_real_size(vtf);
	auto const vtf_new = malloc(max(vtf_size, vtf_size_new));
	if (vtf_new == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	vtf.Save(vtf_new, vtf_size, vtf_size);

	// Remove the 8-bit alpha flag
	vtf_force_72(vtf, vtf_new, true);
	*(unsigned int*)((char*)vtf_new + VTF_HDR_OFF_FLAGS) = flags & ~TEXTUREFLAGS_EIGHTBITALPHA;
	*(int*)((char*)vtf_new + VTF_HDR_OFF_FORMAT) = IMAGE_FORMAT_DXT1;

	// Remove alpha in every mipmap in every frame
	auto m = mips;
	while (m--) {
		auto const mip = mips - m;
		auto const mip_size = vtf.ComputeImageSize(max(width >> m, 1)
		, max(height >> m, 1), depth, IMAGE_FORMAT_DXT5);
		// 16 bytes per block in the original:
		// 8 for color data, 8 for alpha
		auto const blocks = mip_size / 16;
		auto f = frames;
		while (f--) {
			auto const num = frames - f;
			auto data = vtf.GetData(num - 1, 0, 0, m);
			// Copy color data into the new VTF without gaps:
			// 8 bytes per block
			auto b = blocks;
			while (b--) {
				memcpy((char*)vtf_new + off, data + 8, 8);
				data += 16;
				off += 8;
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
