#ifndef DO_API_NO_RETURN
#define DO_API_NO_RETURN(r, n, p) DO_API(r,n,p)
#endif

DO_API(int, il2cpp_init, (const char* domain_name));
DO_API(int, il2cpp_init_utf16, (const uint16_t * domain_name));
DO_API(void, il2cpp_shutdown, ());
DO_API(void, il2cpp_set_config_dir, (const char *config_path));
DO_API(void, il2cpp_set_data_dir, (const char *data_path));
DO_API(void, il2cpp_set_temp_dir, (const char *temp_path));
DO_API(void, il2cpp_set_commandline_arguments, (int argc, const char* const argv[], const char* basedir));
DO_API(void, il2cpp_set_commandline_arguments_utf16, (int argc, const uint16_t * const argv[], const char* basedir));
DO_API(void, il2cpp_set_config_utf16, (const uint16_t * executablePath));
DO_API(void, il2cpp_set_config, (const char* executablePath));

DO_API(void, il2cpp_set_memory_callbacks, (void * callbacks));
DO_API(void*, il2cpp_get_corlib, ());
DO_API(void, il2cpp_add_internal_call, (const char* name, void* method));
DO_API(void*, il2cpp_resolve_icall, (const char* name));

DO_API(void*, il2cpp_alloc, (size_t size));
DO_API(void, il2cpp_free, (void* ptr));

DO_API(void*, il2cpp_array_class_get, (void * element_class, uint32_t rank));
DO_API(uint32_t, il2cpp_array_length, (void * array));
DO_API(uint32_t, il2cpp_array_get_byte_length, (void * array));
DO_API(void*, il2cpp_array_new, (void * elementTypeInfo, uintptr_t length));
DO_API(void*, il2cpp_array_new_specific, (void * arrayTypeInfo, uintptr_t length));
DO_API(void*, il2cpp_array_new_full, (void * array_class, uintptr_t * lengths, uintptr_t * lower_bounds));
DO_API(void*, il2cpp_bounded_array_class_get, (void * element_class, uint32_t rank, bool bounded));
DO_API(int, il2cpp_array_element_size, (void * array_class));

DO_API(void*, il2cpp_assembly_get_image, (void * assembly));

DO_API(void, il2cpp_class_for_each, (void(*klassReportFunc)(void* klass, void* userData), void* userData));
DO_API(void*, il2cpp_class_enum_basetype, (void * klass));
DO_API(bool, il2cpp_class_is_generic, (void * klass));
DO_API(bool, il2cpp_class_is_inflated, (void * klass));
DO_API(bool, il2cpp_class_is_assignable_from, (void * klass, void * oklass));
DO_API(bool, il2cpp_class_is_subclass_of, (void * klass, void * klassc, bool check_interfaces));
DO_API(bool, il2cpp_class_has_parent, (void * klass, void * klassc));
DO_API(void*, il2cpp_class_from_il2cpp_type, (void * type));
DO_API(void*, il2cpp_class_from_name, (void * image, const char* namespaze, const char *name));
DO_API(void*, il2cpp_class_from_system_type, (void * type));
DO_API(void*, il2cpp_class_get_element_class, (void * klass));
DO_API(void*, il2cpp_class_get_events, (void * klass, void* *iter));
DO_API(void*, il2cpp_class_get_fields, (void * klass, void* *iter));
DO_API(void*, il2cpp_class_get_nested_types, (void * klass, void* *iter));
DO_API(void*, il2cpp_class_get_interfaces, (void * klass, void* *iter));
DO_API(void*, il2cpp_class_get_properties, (void * klass, void* *iter));
DO_API(void*, il2cpp_class_get_property_from_name, (void * klass, const char *name));
DO_API(void*, il2cpp_class_get_field_from_name, (void * klass, const char *name));
DO_API(void*, il2cpp_class_get_methods, (void * klass, void* *iter));
DO_API(void*, il2cpp_class_get_method_from_name, (void * klass, const char* name, int argsCount));
DO_API(const char*, il2cpp_class_get_name, (void * klass));
DO_API(void, il2cpp_type_get_name_chunked, (void * type, void(*chunkReportFunc)(void* data, void* userData), void* userData));
DO_API(const char*, il2cpp_class_get_namespace, (void * klass));
DO_API(void*, il2cpp_class_get_parent, (void * klass));
DO_API(void*, il2cpp_class_get_declaring_type, (void * klass));
DO_API(int32_t, il2cpp_class_instance_size, (void * klass));
DO_API(size_t, il2cpp_class_num_fields, (void * enumKlass));
DO_API(bool, il2cpp_class_is_valuetype, (void * klass));
DO_API(int32_t, il2cpp_class_value_size, (void * klass, uint32_t * align));
DO_API(bool, il2cpp_class_is_blittable, (void * klass));
DO_API(int, il2cpp_class_get_flags, (void * klass));
DO_API(bool, il2cpp_class_is_abstract, (void * klass));
DO_API(bool, il2cpp_class_is_interface, (void * klass));
DO_API(int, il2cpp_class_array_element_size, (void * klass));
DO_API(void*, il2cpp_class_from_type, (void * type));
DO_API(void*, il2cpp_class_get_type, (void * klass));
DO_API(uint32_t, il2cpp_class_get_type_token, (void * klass));
DO_API(bool, il2cpp_class_has_attribute, (void * klass, void * attr_class));
DO_API(bool, il2cpp_class_has_references, (void * klass));
DO_API(bool, il2cpp_class_is_enum, (void * klass));
DO_API(void*, il2cpp_class_get_image, (void * klass));
DO_API(const char*, il2cpp_class_get_assemblyname, (void * klass));
DO_API(int, il2cpp_class_get_rank, (void * klass));
DO_API(uint32_t, il2cpp_class_get_data_size, (void * klass));
DO_API(void*, il2cpp_class_get_static_field_data, (void * klass));

DO_API(size_t, il2cpp_class_get_bitmap_size, (void * klass));
DO_API(void, il2cpp_class_get_bitmap, (void * klass, size_t * bitmap));

DO_API(bool, il2cpp_stats_dump_to_file, (const char *path));
DO_API(uint64_t, il2cpp_stats_get_value, (int stat));

DO_API(void*, il2cpp_domain_get, ());
DO_API(void*, il2cpp_domain_assembly_open, (void * domain, const char* name));
DO_API(void**, il2cpp_domain_get_assemblies, (const void * domain, size_t * size));

DO_API_NO_RETURN(void, il2cpp_raise_exception, (void*));
DO_API(void*, il2cpp_exception_from_name_msg, (void * image, const char *name_space, const char *name, const char *msg));
DO_API(void*, il2cpp_get_exception_argument_null, (const char *arg));
DO_API(void, il2cpp_format_exception, (void * ex, char* message, int message_size));
DO_API(void, il2cpp_format_stack_trace, (void * ex, char* output, int output_size));
DO_API(void, il2cpp_unhandled_exception, (void*));
DO_API(void, il2cpp_native_stack_trace, (void * ex, uintptr_t** addresses, int* numFrames, char** imageUUID, char** imageName));

DO_API(int, il2cpp_field_get_flags, (void * field));
DO_API(const char*, il2cpp_field_get_name, (void * field));
DO_API(void*, il2cpp_field_get_parent, (void * field));
DO_API(size_t, il2cpp_field_get_offset, (void * field));
DO_API(void*, il2cpp_field_get_type, (void * field));
DO_API(void, il2cpp_field_get_value, (void * obj, void * field, void *value));
DO_API(void*, il2cpp_field_get_value_object, (void * field, void * obj));
DO_API(bool, il2cpp_field_has_attribute, (void * field, void * attr_class));
DO_API(void, il2cpp_field_set_value, (void * obj, void * field, void *value));
DO_API(void, il2cpp_field_static_get_value, (void * field, void *value));
DO_API(void, il2cpp_field_static_set_value, (void * field, void *value));
DO_API(void, il2cpp_field_set_value_object, (void * instance, void * field, void * value));
DO_API(bool, il2cpp_field_is_literal, (void * field));
DO_API(void, il2cpp_gc_collect, (int maxGenerations));
DO_API(int32_t, il2cpp_gc_collect_a_little, ());
DO_API(void, il2cpp_gc_start_incremental_collection , ());
DO_API(void, il2cpp_gc_disable, ());
DO_API(void, il2cpp_gc_enable, ());
DO_API(bool, il2cpp_gc_is_disabled, ());
DO_API(void, il2cpp_gc_set_mode, (int mode));
DO_API(int64_t, il2cpp_gc_get_max_time_slice_ns, ());
DO_API(void, il2cpp_gc_set_max_time_slice_ns, (int64_t maxTimeSlice));
DO_API(bool, il2cpp_gc_is_incremental, ());
DO_API(int64_t, il2cpp_gc_get_used_size, ());
DO_API(int64_t, il2cpp_gc_get_heap_size, ());
DO_API(void, il2cpp_gc_wbarrier_set_field, (void * obj, void **targetAddress, void *object));
DO_API(bool, il2cpp_gc_has_strict_wbarriers, ());
DO_API(void, il2cpp_gc_set_external_allocation_tracker, (void(*func)(void*, size_t, int)));
DO_API(void, il2cpp_gc_set_external_wbarrier_tracker, (void(*func)(void**)));
DO_API(void, il2cpp_gc_foreach_heap, (void(*func)(void* data, void* userData), void* userData));
DO_API(void, il2cpp_stop_gc_world, ());
DO_API(void, il2cpp_start_gc_world, ());
DO_API(void*, il2cpp_gc_alloc_fixed, (size_t size));
DO_API(void, il2cpp_gc_free_fixed, (void* address));
DO_API(uint32_t, il2cpp_gchandle_new, (void * obj, bool pinned));
DO_API(uint32_t, il2cpp_gchandle_new_weakref, (void * obj, bool track_resurrection));
DO_API(void*, il2cpp_gchandle_get_target , (uint32_t gchandle));
DO_API(void, il2cpp_gchandle_free, (uint32_t gchandle));
DO_API(void , il2cpp_gchandle_foreach_get_target, (void(*func)(void* data, void* userData), void* userData));

DO_API(uint32_t, il2cpp_object_header_size, ());
DO_API(uint32_t, il2cpp_array_object_header_size, ());
DO_API(uint32_t, il2cpp_offset_of_array_length_in_array_object_header, ());
DO_API(uint32_t, il2cpp_offset_of_array_bounds_in_array_object_header, ());
DO_API(uint32_t, il2cpp_allocation_granularity, ());

DO_API(void*, il2cpp_unity_liveness_allocate_struct, (void * filter, int max_object_count, void* callback, void* userdata, void* reallocate));
DO_API(void, il2cpp_unity_liveness_calculation_from_root, (void * root, void* state));
DO_API(void, il2cpp_unity_liveness_calculation_from_statics, (void* state));
DO_API(void, il2cpp_unity_liveness_finalize, (void* state));
DO_API(void, il2cpp_unity_liveness_free_struct, (void* state));

DO_API(void*, il2cpp_method_get_return_type, (void * method));
DO_API(void*, il2cpp_method_get_declaring_type, (void * method));
DO_API(const char*, il2cpp_method_get_name, (void * method));
DO_API(void*, il2cpp_method_get_from_reflection, (const void * method));
DO_API(void*, il2cpp_method_get_object, (void * method, void * refclass));
DO_API(bool, il2cpp_method_is_generic, (void * method));
DO_API(bool, il2cpp_method_is_inflated, (void * method));
DO_API(bool, il2cpp_method_is_instance, (void * method));
DO_API(uint32_t, il2cpp_method_get_param_count, (void * method));
DO_API(void*, il2cpp_method_get_param, (void * method, uint32_t index));
DO_API(void*, il2cpp_method_get_class, (void * method));
DO_API(bool, il2cpp_method_has_attribute, (void * method, void * attr_class));
DO_API(uint32_t, il2cpp_method_get_flags, (void * method, uint32_t * iflags));
DO_API(uint32_t, il2cpp_method_get_token, (void * method));
DO_API(const char*, il2cpp_method_get_param_name, (void * method, uint32_t index));

#if IL2CPP_ENABLE_PROFILER

DO_API(void, il2cpp_profiler_install, (void * prof, void* shutdown_callback));
DO_API(void, il2cpp_profiler_set_events, (int events));
DO_API(void, il2cpp_profiler_install_enter_leave, (void* enter, void* fleave));
DO_API(void, il2cpp_profiler_install_allocation, (void* callback));
DO_API(void, il2cpp_profiler_install_gc, (void* callback, void* heap_resize_callback));
DO_API(void, il2cpp_profiler_install_fileio, (void* callback));
DO_API(void, il2cpp_profiler_install_thread, (void* start, void* end));

#endif

DO_API(uint32_t, il2cpp_property_get_flags, (void * prop));
DO_API(void*, il2cpp_property_get_get_method, (void * prop));
DO_API(void*, il2cpp_property_get_set_method, (void * prop));
DO_API(const char*, il2cpp_property_get_name, (void * prop));
DO_API(void*, il2cpp_property_get_parent, (void * prop));

DO_API(void*, il2cpp_object_get_class, (void * obj));
DO_API(uint32_t, il2cpp_object_get_size, (void * obj));
DO_API(void*, il2cpp_object_get_virtual_method, (void * obj, void * method));
DO_API(void*, il2cpp_object_new, (void * klass));
DO_API(void*, il2cpp_object_unbox, (void * obj));

DO_API(void*, il2cpp_value_box, (void * klass, void* data));

DO_API(void, il2cpp_monitor_enter, (void * obj));
DO_API(bool, il2cpp_monitor_try_enter, (void * obj, uint32_t timeout));
DO_API(void, il2cpp_monitor_exit, (void * obj));
DO_API(void, il2cpp_monitor_pulse, (void * obj));
DO_API(void, il2cpp_monitor_pulse_all, (void * obj));
DO_API(void, il2cpp_monitor_wait, (void * obj));
DO_API(bool, il2cpp_monitor_try_wait, (void * obj, uint32_t timeout));

DO_API(void*, il2cpp_runtime_invoke, (void * method, void *obj, void **params, void **exc));
DO_API(void*, il2cpp_runtime_invoke_convert_args, (void * method, void *obj, void **params, int paramCount, void **exc));
DO_API(void, il2cpp_runtime_class_init, (void * klass));
DO_API(void, il2cpp_runtime_object_init, (void * obj));

DO_API(void, il2cpp_runtime_object_init_exception, (void * obj, void** exc));

DO_API(void, il2cpp_runtime_unhandled_exception_policy_set, (int value));

DO_API(int32_t, il2cpp_string_length, (void * str));
DO_API(void*, il2cpp_string_chars, (void * str));
DO_API(void*, il2cpp_string_new, (const char* str));
DO_API(void*, il2cpp_string_new_len, (const char* str, uint32_t length));
DO_API(void*, il2cpp_string_new_utf16, (const uint16_t * text, int32_t len));
DO_API(void*, il2cpp_string_new_wrapper, (const char* str));
DO_API(void*, il2cpp_string_intern, (void * str));
DO_API(void*, il2cpp_string_is_interned, (void * str));

DO_API(void*, il2cpp_thread_current, ());
DO_API(void*, il2cpp_thread_attach, (void * domain));
DO_API(void, il2cpp_thread_detach, (void * thread));

DO_API(void**, il2cpp_thread_get_all_attached_threads, (size_t * size));
DO_API(bool, il2cpp_is_vm_thread, (void * thread));

DO_API(void, il2cpp_current_thread_walk_frame_stack, (void* func, void* user_data));
DO_API(void, il2cpp_thread_walk_frame_stack, (void * thread, void* func, void* user_data));
DO_API(bool, il2cpp_current_thread_get_top_frame, (void * frame));
DO_API(bool, il2cpp_thread_get_top_frame, (void * thread, void * frame));
DO_API(bool, il2cpp_current_thread_get_frame_at, (int32_t offset, void * frame));
DO_API(bool, il2cpp_thread_get_frame_at, (void * thread, int32_t offset, void * frame));
DO_API(int32_t, il2cpp_current_thread_get_stack_depth, ());
DO_API(int32_t, il2cpp_thread_get_stack_depth, (void * thread));
DO_API(void, il2cpp_override_stack_backtrace, (void* stackBacktraceFunc));

DO_API(void*, il2cpp_type_get_object, (void * type));
DO_API(int, il2cpp_type_get_type, (void * type));
DO_API(void*, il2cpp_type_get_class_or_element_class, (void * type));
DO_API(char*, il2cpp_type_get_name, (void * type));
DO_API(bool, il2cpp_type_is_byref, (void * type));
DO_API(uint32_t, il2cpp_type_get_attrs, (void * type));
DO_API(bool, il2cpp_type_equals, (void * type, void * otherType));
DO_API(char*, il2cpp_type_get_assembly_qualified_name, (void * type));
DO_API(bool, il2cpp_type_is_static, (void * type));
DO_API(bool, il2cpp_type_is_pointer_type, (void * type));

DO_API(void*, il2cpp_image_get_assembly, (void * image));
DO_API(const char*, il2cpp_image_get_name, (void * image));
DO_API(const char*, il2cpp_image_get_filename, (void * image));
DO_API(void*, il2cpp_image_get_entry_point, (void * image));

DO_API(size_t, il2cpp_image_get_class_count, (void * image));
DO_API(void*, il2cpp_image_get_class, (void * image, size_t index));

DO_API(void*, il2cpp_capture_memory_snapshot, ());
DO_API(void, il2cpp_free_captured_memory_snapshot, (void * snapshot));

DO_API(void, il2cpp_set_find_plugin_callback, (void* method));

DO_API(void, il2cpp_register_log_callback, (void* method));

DO_API(void, il2cpp_debugger_set_agent_options, (const char* options));
DO_API(bool, il2cpp_is_debugger_attached, ());
DO_API(void, il2cpp_register_debugger_agent_transport, (void * debuggerTransport));

DO_API(bool, il2cpp_debug_get_method_info, (void*, void * methodDebugInfo));

DO_API(void, il2cpp_unity_install_unitytls_interface, (const void* unitytlsInterfaceStruct));

DO_API(void*, il2cpp_custom_attrs_from_class, (void * klass));
DO_API(void*, il2cpp_custom_attrs_from_method, (void * method));

DO_API(void*, il2cpp_custom_attrs_get_attr, (void * ainfo, void * attr_klass));
DO_API(bool, il2cpp_custom_attrs_has_attr, (void * ainfo, void * attr_klass));
DO_API(void*,  il2cpp_custom_attrs_construct, (void * cinfo));

DO_API(void, il2cpp_custom_attrs_free, (void * ainfo));

DO_API(void, il2cpp_class_set_userdata, (void * klass, void* userdata));
DO_API(int, il2cpp_class_get_userdata_offset, ());

DO_API(void, il2cpp_set_default_thread_affinity, (int64_t affinity_mask));
