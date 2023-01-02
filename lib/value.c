#include "value.h"

ThsnResult thsn_value_type(ThsnSlice data_slice, ThsnValueHandle value_handle,
                           ThsnValueType* value_type);

ThsnResult thsn_value_read_bool(ThsnSlice slice, ThsnValueHandle value_handle,
                                bool* value);

ThsnResult thsn_value_read_number(ThsnSlice data_slice,
                                  ThsnValueHandle value_handle, double* value);

ThsnResult thsn_value_read_string_ex(ThsnSlice data_slice,
                                     ThsnValueHandle value_handle,
                                     ThsnSlice* string_slice,
                                     size_t* consumed_size);

ThsnResult thsn_value_read_string(ThsnSlice data_slice,
                                  ThsnValueHandle value_handle,
                                  ThsnSlice* string_slice);

ThsnResult thsn_value_read_composite(ThsnSlice data_slice,
                                     ThsnValueHandle value_handle,
                                     ThsnTagType expected_type,
                                     ThsnSlice* elements_table);

size_t thsn_value_read_array(ThsnSlice data_slice, ThsnValueHandle value_handle,
                             ThsnValueArrayTable* array_table);

size_t thsn_value_array_length(ThsnValueArrayTable array_table);

ThsnResult thsn_value_array_element_handle(ThsnValueArrayTable array_table,
                                           size_t element_no,
                                           ThsnValueHandle* handle);

ThsnResult thsn_value_array_consume_element(ThsnValueArrayTable* array_table,
                                            ThsnValueHandle* element_handle);

ThsnResult thsn_value_read_object(ThsnSlice data_slice,
                                  ThsnValueHandle value_handle,
                                  ThsnValueObjectTable* object_table);

size_t thsn_value_object_length(ThsnValueObjectTable object_table);

ThsnResult thsn_value_object_read_kv(ThsnSlice data_slice,
                                     ThsnValueHandle kv_handle,
                                     ThsnSlice* key_slice,
                                     ThsnValueHandle* value_handle);

ThsnResult thsn_value_object_element_handle(ThsnSlice data_slice,
                                            ThsnValueObjectTable object_table,
                                            size_t element_no,
                                            ThsnSlice* key_slice,
                                            ThsnValueHandle* value_handle);

ThsnResult thsn_value_object_consume_element(ThsnSlice data_slice,
                                             ThsnValueObjectTable* object_table,
                                             ThsnSlice* key_slice,
                                             ThsnValueHandle* value_handle);

ThsnResult thsn_value_object_index(ThsnSlice data_slice,
                                   ThsnValueObjectTable object_table,
                                   ThsnSlice key_slice,
                                   ThsnValueHandle* handle);
