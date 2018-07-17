#pragma once

#include <mutex>

#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/filesystem.hpp>

namespace itk {

template<class TInputImage, class TOutputImage>
void MedialAxisImageFilter<TInputImage, TOutputImage>::GenerateData()
{
	// load lookup table once
	static LookupTable lookupTable;
	static std::once_flag flag;

	std::call_once(flag, [&]() {
		namespace fs = boost::filesystem;
		boost::system::error_code ec;
		auto exe_path = boost::dll::program_location(ec);
		auto lut_path = exe_path.parent_path() / fs::path("LookupTables") / fs::path("Thinning_MedialAxis.bin");
		if (fs::exists(lut_path, ec))
		{
			lookupTable.readFile(lut_path.string());
		}
		else
		{
			throw itk::ExceptionObject("", 0, "Could not load lookup table");
		}
	});

	Superclass::SetLookupTable(lookupTable);

	Superclass::GenerateData();
}

} // namespace itk
