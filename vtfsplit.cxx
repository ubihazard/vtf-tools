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
		printf("Usage: %s animation.vtf\n", app);
		return (argc == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// Load the animated texture
	auto anim = CVTFFile{};
	if (!anim.Load(argv[0])) {
		printf("Error: couldn't load \"%s\"\n", argv[0]);
		goto usage;
	}

	// Get the texture parameters
	auto const width = anim.GetWidth();
	auto const height = anim.GetHeight();
	auto const depth = anim.GetDepth();
	auto const mips = anim.GetMipmapCount();
	auto const frames = anim.GetFrameCount();
	auto const format = anim.GetFormat();

	// Multiple frames expected
	if (frames == 1) {
		printf("Error: file \"%s\" is not an animation\n", argv[0]);
		return EXIT_FAILURE;
	}

	// Calculate the size of a single frame
	vlUInt img_size_new = 0;
	auto m = mips;
	while (m--) {
		auto const mip_size = anim.ComputeImageSize(max(width >> m, 1)
		, max(height >> m, 1), depth, format);
		img_size_new += mip_size;
	}

	// Allocate memory for a single VTF
	auto const thumb_size = anim.GetThumbnailDataSize();
	auto const vtf_size_new = VTF_HDR_SIZE + thumb_size + img_size_new;
	auto const vtf_mem = malloc(vtf_size_new);
	if (vtf_mem == NULL) {
nomem:
		printf("Error: out of memory\n");
		goto fail;
	}

	// Copy the header
	auto const vtf_hdr = calloc(VTF_HDR_SIZE, 1);
	if (vtf_hdr == NULL) {
		goto nomem;
	}
	auto const hdr = anim.GetHeaderData();
	memcpy(vtf_hdr, hdr, min(VTF_HDR_SIZE, hdr->HeaderSize));
	vtf_force_72(anim, vtf_hdr, false);

	// Set the number of frames and other metadata
	*(short*)((char*)vtf_hdr + VTF_HDR_OFF_FRAMES) = 1;
	*(short*)((char*)vtf_hdr + VTF_HDR_OFF_START) = 0;

	// Write individual frames
	char fname[256];
	auto dot = strrchr(argv[0], '.');
	if (dot != NULL) {
		dot[0] = '\0';
	}
	if (strlen(argv[0]) > sizeof(fname) - 34) {
		goto fail;
	}
	if (mkdir(argv[0]) == -1) {
		perror("Error");
		goto fail;
	}

	auto f = frames;
	while (f--) {
		// Write the next frame
		size_t off = VTF_HDR_SIZE;
		auto const num = frames - f;
		memcpy(vtf_mem, vtf_hdr, off);
		if (f == frames - 1) {
			memcpy((char*)vtf_mem + off, anim.GetThumbnailData(), thumb_size);
			off += thumb_size;

			// No thumbnail for subsequent frames
			*((char*)vtf_hdr + VTF_HDR_OFF_THUMB_FMT) = -1;
			*((char*)vtf_hdr + VTF_HDR_OFF_THUMB_W) = 0;
			*((char*)vtf_hdr + VTF_HDR_OFF_THUMB_H) = 0;
		}

		// Write each mipmap
		m = mips;
		while (m--) {
			auto const data = anim.GetData(num - 1, 0, 0, m);
			auto const mip_size = anim.ComputeImageSize(max(width >> m, 1)
			, max(height >> m, 1), depth, format);
			memcpy((char*)vtf_mem + off, data, mip_size);
			off += mip_size;
		}

		// Write the file
		sprintf(fname, "%s/%d.vtf", argv[0], num);
		auto const f = fopen(fname, "wb");
		if (f == NULL) {
			printf("Error: couldn't write \"%s\"\n", fname);
			goto fail;
		}
		fwrite(vtf_mem, 1, off, f);
		fclose(f);
	}

	// Cleanup
	free(vtf_mem);

	return EXIT_SUCCESS;

fail:
	free(vtf_mem);

	return EXIT_FAILURE;
}
