#include <chrono>
#include <cstring>
#include <iostream>

#include "simdjson.h"
#include "threason.h"
#include "unordered_map"
#include "yyjson.h"

int64_t simdjson_dom(simdjson::padded_string &json, size_t bunch_no,
                     size_t status_no) {
    simdjson::dom::parser parser{};
    auto doc = parser.parse(json);
    return doc.get_array().at(bunch_no)["statuses"].get_array().at(
        status_no)["retweet_count"];
}

int64_t yyjson_dom(char *json, size_t json_size, size_t bunch_no,
                   size_t status_no) {
    yyjson_doc *doc = yyjson_read(json, json_size, YYJSON_READ_NOFLAG);
    yyjson_val *root = yyjson_doc_get_root(doc);
    yyjson_val *bunch = yyjson_arr_get(root, bunch_no);
    yyjson_val *statuses = yyjson_obj_get(bunch, "statuses");
    yyjson_val *status = yyjson_arr_get(statuses, status_no);
    yyjson_val *retweet_count = yyjson_obj_get(status, "retweet_count");
    return yyjson_get_sint(retweet_count);
}

template <typename ParserFn>
int64_t threason_dom(ParserFn parser_fn, ThsnSlice json, size_t bunch_no,
                     size_t status_no) {
    /* Ignoring errors */
    ThsnDocument *document;
    parser_fn(&json, &document);
    ThsnValueArrayTable top_array_table;
    thsn_document_read_array(document, thsn_value_handle_first(),
                             &top_array_table);
    ThsnValueHandle bunch_object_handle;
    thsn_document_index_array_element(document, top_array_table, bunch_no,
                                      &bunch_object_handle);
    ThsnValueObjectTable bunch_object_sorted_table;
    thsn_document_read_object_sorted(document, bunch_object_handle,
                                     &bunch_object_sorted_table);
    ThsnValueHandle statuses_array_handle;
    thsn_document_object_index(document, bunch_object_sorted_table,
                               thsn_slice_from_c_str("statuses"),
                               &statuses_array_handle);
    ThsnValueArrayTable statuses_array_table;
    thsn_document_read_array(document, statuses_array_handle,
                             &statuses_array_table);
    ThsnValueHandle status_object_handle;
    thsn_document_index_array_element(document, statuses_array_table, status_no,
                                      &status_object_handle);
    ThsnValueObjectTable status_object_sorted_table;
    thsn_document_read_object_sorted(document, status_object_handle,
                                     &status_object_sorted_table);
    ThsnValueHandle retweet_count_handle;
    thsn_document_object_index(document, status_object_sorted_table,
                               thsn_slice_from_c_str("retweet_count"),
                               &retweet_count_handle);
    double retweet_count;
    thsn_document_read_number(document, retweet_count_handle, &retweet_count);
    thsn_document_free(&document);
    return (int64_t)retweet_count;
}

int main(int argc, char **argv) {
    enum class Bench {
        simdjson,
        yyjson,
        threason_1t,
        threason_2t,
        threason_4t,
        threason_8t,
    };

    Bench bench = Bench::threason_1t;
    size_t bunch_no = 0;
    size_t status_no = 4;

    if (argc > 1) {
        const std::unordered_map<std::string, Bench> benches = {
            {"simdjson", Bench::simdjson},
            {"yyjson", Bench::yyjson},
            {"threason_1t", Bench::threason_1t},
            {"threason_2t", Bench::threason_2t},
            {"threason_4t", Bench::threason_4t},
            {"threason_8t", Bench::threason_8t},
        };
        if (auto b = benches.find(argv[1]); b != benches.end()) {
            bench = b->second;
        }
    }

    if (argc > 2) {
        bunch_no = std::stoll(argv[2]);
    }

    if (argc > 3) {
        status_no = std::stoll(argv[3]);
    }

    std::cin >> std::noskipws;
    std::istream_iterator<char> cin_begin(std::cin);
    std::istream_iterator<char> cin_end;
    std::string json_string(cin_begin, cin_end);
    simdjson::padded_string padded_json =
        simdjson::padded_string(std::string_view(json_string));
    ThsnSlice json_slice =
        thsn_slice_make((const char *)json_string.data(), json_string.size());

    int64_t result = 0;
    auto start_time = std::chrono::steady_clock::now();
    switch (bench) {
        case Bench::simdjson: {
            std::cout << "Using simdjson" << std::endl;
            result = simdjson_dom(padded_json, bunch_no, status_no);
            break;
        }
        case Bench::yyjson: {
            std::cout << "Using yyjson" << std::endl;
            result = yyjson_dom(json_string.data(), json_string.size(),
                                bunch_no, status_no);
            break;
        };
        case Bench::threason_1t: {
            std::cout << "Using threason_1t" << std::endl;
            result = threason_dom(thsn_document_parse, json_slice, bunch_no,
                                  status_no);
            break;
        }
        case Bench::threason_2t: {
            std::cout << "Using threason_2t" << std::endl;
            result = threason_dom(
                [](auto json_slice, auto document) {
                    return thsn_document_parse_multithreaded(json_slice,
                                                             document, 2);
                },
                json_slice, bunch_no, status_no);
            break;
        }
        case Bench::threason_4t: {
            std::cout << "Using threason_4t" << std::endl;
            result = threason_dom(
                [](auto json_slice, auto document) {
                    return thsn_document_parse_multithreaded(json_slice,
                                                             document, 4);
                },
                json_slice, bunch_no, status_no);
            break;
        }
        case Bench::threason_8t: {
            std::cout << "Using threason_8t" << std::endl;
            result = threason_dom(
                [](auto json_slice, auto document) {
                    return thsn_document_parse_multithreaded(json_slice,
                                                             document, 8);
                },
                json_slice, bunch_no, status_no);
            break;
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(
        end_time - start_time);
    std::cout << "Result " << result << std::endl;
    std::cout << "Elapsed " << diff.count() << "us" << std::endl;
    return 0;
}
