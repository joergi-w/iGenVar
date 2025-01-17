#include <seqan3/alphabet/cigar/cigar.hpp>
#include <seqan3/alphabet/nucleotide/dna5.hpp>
#include <seqan3/core/debug_stream.hpp>
#include <seqan3/io/sequence_file/output.hpp>

#include "iGenVar.hpp"              // for global variable gVerbose
#include "structures/breakend.hpp"  // for class Breakend
#include "structures/junction.hpp"  // for class Junction

using seqan3::operator""_cigar_operation;
using seqan3::operator""_dna5;

void analyze_cigar(std::string const & read_name,
                   std::string const & chromosome,
                   int32_t const query_start_pos,
                   std::vector<seqan3::cigar> & cigar_string,
                   seqan3::dna5_vector const & query_sequence,
                   std::vector<Junction> & junctions,
                   int32_t const min_length)
{
    // Step through CIGAR string and store current position in reference and read
    int32_t pos_ref = query_start_pos;
    int32_t pos_read = 0;

    for (seqan3::cigar & pair : cigar_string)
    {
        using seqan3::get;
        int32_t length = get<0>(pair);
        seqan3::cigar::operation operation = get<1>(pair);
        size_t tandem_dup_count = 0;
        if (operation == 'M'_cigar_operation || operation == '='_cigar_operation || operation == 'X'_cigar_operation)
        {
            pos_ref += length;
            pos_read += length;
        }
        else if (operation == 'I'_cigar_operation) // I: Insertion (gap in the reference sequence)
        {
            if (length >= min_length)
            {
                // Insertions cause one junction from the insertion location to the next base
                auto inserted_bases = query_sequence | seqan3::views::slice(pos_read, pos_read + length);
                Junction new_junction{Breakend{chromosome, pos_ref - 1, strand::forward},
                                      Breakend{chromosome, pos_ref, strand::forward},
                                      inserted_bases,
                                      tandem_dup_count,
                                      read_name};
                if (gVerbose)
                    seqan3::debug_stream << "INS: " << new_junction << "\n";
                junctions.push_back(std::move(new_junction));
            }
            pos_read += length;
        }
        else if (operation == 'D'_cigar_operation)
        {
            if (length >= min_length)
            {
                // Deletions cause one junction from its start to its end
                Junction new_junction{Breakend{chromosome, pos_ref - 1, strand::forward},
                                      Breakend{chromosome, pos_ref + length, strand::forward},
                                      ""_dna5,
                                      tandem_dup_count,
                                      read_name};
                if (gVerbose)
                    seqan3::debug_stream << "DEL: " << new_junction << "\n";
                junctions.push_back(std::move(new_junction));
            }
            pos_ref += length;
        }
        else if (operation == 'S'_cigar_operation)
        {
            pos_read += length;
        }
        else // other possible cigar operations: H, N, P
        {
            // seqan3::debug_stream << "Unhandled operation " << operation << std::endl;
        }

    }
}
