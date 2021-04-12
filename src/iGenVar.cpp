#include <seqan3/argument_parser/argument_parser.hpp>   // for seqan3::argument_parser
#include <seqan3/core/debug_stream.hpp>                 // for seqan3::debug_stream

#include "variant_detection/variant_detection.hpp"
#include "variant_detection/validator.hpp"            // for class EnumValidator

void initialize_argument_parser(seqan3::argument_parser & parser, cmd_arguments & args)
{
    parser.info.author = "Lydia Buntrock, David Heller, Joshua Kim";
    parser.info.app_name = "iGenVar";
    parser.info.man_page_title = "Short and Long Read SV Caller";
    parser.info.short_description = "Detect genomic variants in a read alignment file";
    parser.info.version = "0.0.2";
    parser.info.date = "30-03-2021";    // last update
    parser.info.email = "lydia.buntrock@fu-berlin.de";
    parser.info.long_copyright = "long_copyright";
    parser.info.short_copyright = "short_copyright";
    parser.info.url = "https://github.com/seqan/iGenVar/";

    // Validatiors:
    EnumValidator<detection_methods> detection_method_validator{seqan3::enumeration_names<detection_methods>
                                                                | std::views::values};
    EnumValidator<clustering_methods> clustering_method_validator{seqan3::enumeration_names<clustering_methods>
                                                                  | std::views::values};
    EnumValidator<refinement_methods> refinement_method_validator{seqan3::enumeration_names<refinement_methods>
                                                                  | std::views::values};

    // Options - Input / Output:
    parser.add_option(args.alignment_short_reads_file_path,
                      'i', "input_short_reads",
                      "Input short read alignments in SAM or BAM format (Illumina).",
                      seqan3::option_spec::standard,
                      seqan3::input_file_validator{{"sam", "bam"}} );
    parser.add_option(args.alignment_long_reads_file_path,
                      'j', "input_long_reads",
                      "Input long read alignments in SAM or BAM format (PacBio, Oxford Nanopore, ...).",
                      seqan3::option_spec::standard,
                      seqan3::input_file_validator{{"sam", "bam"}} );
    parser.add_option(args.output_file_path, 'o', "output",
                      "The path of the vcf output file. If no path is given, will output to standard output.",
                      seqan3::option_spec::standard,
                      seqan3::output_file_validator{seqan3::output_file_open_options::open_or_create, {"vcf"}});

    // Options - Methods:
    parser.add_option(args.methods, 'm', "method", "Choose the detection method(s) to be used.",
                      seqan3::option_spec::advanced, detection_method_validator);
    parser.add_option(args.clustering_method, 'c', "clustering_method", "Choose the clustering method to be used.",
                      seqan3::option_spec::advanced, clustering_method_validator);
    parser.add_option(args.refinement_method, 'r', "refinement_method", "Choose the refinement method to be used.",
                      seqan3::option_spec::advanced, refinement_method_validator);

    // Options - SV specifications:
    parser.add_option(args.min_var_length, 'l', "min_var_length",
                      "Specify what should be the minimum length of your SVs to be detected (default 30 bp).",
                      seqan3::option_spec::advanced);
}

int main(int argc, char ** argv)
{
    seqan3::argument_parser myparser{"iGenVar", argc, argv};    // initialise myparser
    cmd_arguments args{};
    initialize_argument_parser(myparser, args);

    // Parse the given arguments and catch possible errors.
    try
    {
        myparser.parse();                                               // trigger command line parsing
    }
    catch (seqan3::argument_parser_error const & ext)                   // catch user errors
    {
        seqan3::debug_stream << "[Error] " << ext.what() << '\n';       // customise your error message
        return -1;
    }

    // Check if we have at least one input file.
    if (args.alignment_short_reads_file_path == "" && args.alignment_long_reads_file_path == "")
    {
        seqan3::debug_stream << "[Error] You need to input at least one sam/bam file.\n"
                             << "Please use -i or -input_short_reads to pass a short read file "
                                "or -j or -input_long_reads for a long read file.\n";
        return -1;
    }

    // Check that method selection contains no duplicates.
    std::vector<detection_methods> unique_methods{args.methods};
    std::ranges::sort(unique_methods);
    unique_methods.erase(std::unique(unique_methods.begin(), unique_methods.end()), unique_methods.end());
    if (args.methods.size() > unique_methods.size())
    {
        seqan3::debug_stream << "[Error] The same detection method was selected multiple times.\n";
        seqan3::debug_stream << "Methods to be used: " << args.methods << '\n';
        return -1;
    }

    detect_variants_in_alignment_file(args);

    return 0;
}
