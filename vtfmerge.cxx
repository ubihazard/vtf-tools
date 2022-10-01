#include "vtfshared.hxx"

int main (int argc, char** argv)
{
	using namespace VTFLib;

	// Get the number of animation frames
	const auto* const app = argv[0];
	++argv, --argc;

	if (argc < 2) {
		printf("Error: at least two textures must be specified (got %d)\n", argc);
usage:
		printf("Usage: %s frame1.vtf frame2.vtf [frame3.vtf ...]\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the first texture
	auto const dir = argv[0];
	auto frame = CVTFFile{};
	if (!frame.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}

	// The first texture defines parameters for the entire animation.
	// All other textures must be identical in size, mimpmap count, and format.
	auto const width = frame.GetWidth();
	auto const height = frame.GetHeight();
	auto const depth = frame.GetDepth();
	auto const mips = frame.GetMipmapCount();
	auto const frames = frame.GetFrameCount();
	auto const format = frame.GetFormat();

	// Individual frame textures cannot have multiple frames
	if (frames != 1) {
		printf("Error: frame \"%s\": contains multiple frames on its own\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Calculate the size of resulting
	// animated texture image data
	auto const thumb_size = frame.GetThumbnailDataSize();
	auto const off = VTF_HDR_SIZE + thumb_size;
	size_t* const mip_lut = re_cast<size_t*>(malloc(sizeof(mip_lut[0]) * (mips + 1)));
	if (mip_lut == NULL) {
		printf("Error: out of memory\n");
		return EXIT_FAILURE;
	}
	mip_lut[0] = off;
	vlUInt img_size_new = 0;
	auto m = mips;
	while (m--) {
		auto const mip = mips - m;
		auto const mip_size = frame.ComputeImageSize(max(width >> m, 1)
		, max(height >> m, 1), depth, format);
		mip_lut[mip] = mip_lut[mip - 1] + mip_size * argc;
		img_size_new += mip_size;
	}

	// Allocate the memory for entire animated VTF
	auto vtf_size = vtf_get_real_size(frame);
	auto const vtf_size_new = off + img_size_new * argc;
	auto const vtf_mem = malloc(max(vtf_size, vtf_size_new));
	if (vtf_mem == NULL) {
		printf("Error: out of memory\n");
		goto fail;
	}

	// Write the first frame
	auto const img_size = frame.GetImageDataSize();
	frame.Save(vtf_mem, vtf_size, vtf_size);

	// Set the number of frames and other metadata
	vtf_force_72(frame, vtf_mem, true);
	*(short*)((char*)vtf_mem + VTF_HDR_OFF_FRAMES) = argc;
	*(short*)((char*)vtf_mem + VTF_HDR_OFF_START) = 0;

	// Load the rest of textures
	auto const total = argc;
	while (true) {
		--argc;
		auto const num = total - argc;

		// Write the next frame for each mipmap
		m = mips;
		while (m--) {
			auto const mip = mips - m;
			auto const data = frame.GetData(0, 0, 0, m);
			auto const mip_size = frame.ComputeImageSize(max(width >> m, 1)
			, max(height >> m, 1), depth, format);
			auto const mip_off = mip_lut[mip - 1]
			+ mip_size * (num - 1);
			memcpy((char*)vtf_mem + mip_off, data, mip_size);
		}
		if (argc == 0) {
			break;
		}

		// Load the next frame
		++argv;
		if (!frame.Load(argv[0])) {
			printf("Error: couldn't load \"%s\"\n", argv[0]);
			goto fail;
		}

		// Validate against the first frame
		if (frame.GetWidth() != width) {printf("Error: frame \"%s\": inconsistent width\n", argv[0]); goto fail;}
		if (frame.GetHeight() != height) {printf("Error: frame \"%s\": inconsistent height\n", argv[0]); goto fail;}
		if (frame.GetDepth() != depth) {printf("Error: frame \"%s\": inconsistent depth\n", argv[0]); goto fail;}
		if (frame.GetMipmapCount() != mips) {printf("Error: frame \"%s\": inconsistent mipmap count\n", argv[0]); goto fail;}
		if (frame.GetFormat() != format) {printf("Error: frame \"%s\": inconsistent format\n", argv[0]); goto fail;}

		// Individual frame textures must not be animated themselves
		if (frame.GetFrameCount() != 1) {printf("Error: frame \"%s\": contains multiple frames on its own\n", argv[0]); goto fail;}
	}

	// Get the file name
	char fname[256];
	auto dot = strrchr(dir, '\\');
	if (dot == NULL) {
		dot = strrchr(dir, '/');
	}
	if (dot != NULL) {
		dot[0] = '\0';
	}
	if (strlen(dir) > sizeof(fname) - 5) {
		goto fail;
	}
	sprintf(fname, "%s.vtf", dir);

	// Write the file
	auto const f = fopen(fname, "wb");
	if (f == NULL) {
		printf("Error: couldn't write \"%s\"\n", fname);
		goto fail;
	}
	fwrite(vtf_mem, 1, vtf_size_new, f);
	fclose(f);

	// Cleanup
	free(vtf_mem);
	free(mip_lut);

	return EXIT_SUCCESS;

fail:
	free(vtf_mem);
	free(mip_lut);

	return EXIT_FAILURE;
}
