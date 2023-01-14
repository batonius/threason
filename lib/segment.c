#include "segment.h"

ThsnTag thsn_tag_make(ThsnTagType type, ThsnTagSize size);

ThsnTagType thsn_tag_type(ThsnTag tag);

ThsnTagSize thsn_tag_size(ThsnTag tag);

ThsnResult thsn_segment_store_tagged_value(ThsnSegment* /*mut*/ segment,
                                           ThsnTag tag, ThsnSlice value_slice);

ThsnResult thsn_segment_store_null(ThsnSegment* /*mut*/ segment);

ThsnResult thsn_segment_store_bool(ThsnSegment* /*mut*/ segment, bool value);

ThsnResult thsn_segment_store_double(ThsnSegment* /*mut*/ segment,
                                     double value);

ThsnResult thsn_segment_store_int(ThsnSegment* /*mut*/ segment,
                                  long long value);

ThsnResult thsn_segment_store_value_handle(ThsnSegment* /*mut*/ segment,
                                           ThsnValueHandle value_handle);

ThsnResult thsn_segment_store_string(ThsnSegment* /*mut*/ segment,
                                     ThsnSlice string_slice);

ThsnResult thsn_segment_read_string_from_slice(
    ThsnSlice slice, ThsnSlice* /*out*/ string_slice,
    size_t* /*maybe out*/ stored_length);

int thsn_compare_kv_keys(const void* a, const void* b, void* data);

ThsnResult thsn_segment_sort_elements_table(ThsnMutSlice elements_table,
                                            ThsnSlice result_slice);

ThsnResult thsn_segment_read_tagged_value(ThsnSegmentSlice segment_slice,
                                          size_t offset,
                                          ThsnTag* /*out*/ value_tag,
                                          ThsnSlice* /*out*/ value_slice);

ThsnResult thsn_segment_update_tag(ThsnSegmentMutSlice segment_mut_slice,
                                   size_t offset, ThsnTag tag);

ThsnResult thsn_segment_read_bool(ThsnSegmentSlice segment_slice, size_t offset,
                                  bool* /*out*/ value);

ThsnResult thsn_segment_read_number(ThsnSegmentSlice segment_slice,
                                    size_t offset, double* /*out*/ value);

ThsnResult thsn_segment_read_string_ex(ThsnSegmentSlice segment_slice,
                                       size_t offset,
                                       ThsnSlice* /*out*/ string_slice,
                                       size_t* /*maybe out*/ consumed_size);

ThsnResult thsn_segment_read_composite(ThsnSegmentMutSlice segment_slice,
                                       size_t offset, ThsnTagType expected_type,
                                       ThsnSlice* /*out*/ elements_table,
                                       bool read_sorted_table);

ThsnResult thsn_segment_composite_index_element_offset(
    ThsnSlice elements_table, size_t element_no,
    size_t* /*out*/ element_offset);

ThsnResult thsn_segment_composite_consume_element_offset(
    ThsnSlice* /*mut*/ elements_table, size_t* /*out*/ element_offset);

ThsnResult thsn_segment_object_read_kv(ThsnSegmentSlice segment_slice,
                                       size_t offset,
                                       ThsnSlice* /*out*/ key_slice,
                                       size_t* /*out*/ element_offset);

ThsnResult thsn_segment_object_index(ThsnSegmentSlice segment_slice,
                                     ThsnSlice sorted_elements_table,
                                     ThsnSlice key_slice,
                                     size_t* /*out*/ element_offset,
                                     bool* /*out*/ found);
