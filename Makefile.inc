####################### Antaris dependencies ####################################
VENDOR_cJSON_INCLUDES= -I vendor/cJSON/interface/
GRPC_CPP_ADDITIONAL_LIBS = -L /usr/local/antaris/grpc/lib64/ \
			   -L /usr/local/antaris/grpc/lib/ \
			   -lprotobuf -lgrpc++ -lgrpc -lgrpc++_reflection \
			   -lgpr -lupb -labsl_bad_optional_access -labsl_cord \
			   -labsl_raw_logging_internal -labsl_cordz_info \
			   -labsl_cordz_handle -labsl_base -labsl_spinlock_wait \
			   -labsl_synchronization -labsl_base -labsl_malloc_internal \
			   -labsl_synchronization -labsl_symbolize \
			   -labsl_debugging_internal -labsl_demangle_internal \
			   -labsl_time -labsl_time_zone -labsl_int128 \
			   -labsl_graphcycles_internal -labsl_stacktrace \
			   -labsl_debugging_internal -labsl_cordz_functions \
			   -labsl_exponential_biased -labsl_cord_internal \
			   -labsl_throw_delegate -labsl_strings -labsl_strings_internal \
			   -labsl_status -labsl_str_format_internal -labsl_statusor \
			   -labsl_bad_variant_access -lre2 -lcares \
			   -laddress_sorting -labsl_hash -labsl_city \
			   -labsl_low_level_hash -labsl_random_internal_randen_slow \
			   -labsl_random_internal_platform \
			   -labsl_random_internal_randen_hwaes_impl \
			   -labsl_random_internal_pool_urbg \
			   -labsl_random_internal_seed_material \
			   -labsl_random_seed_gen_exception \
			   -labsl_random_internal_randen \
			   -labsl_random_internal_randen_hwaes -lpthread -lssl -lcrypto -lz
GRPC_CPP_ADDITIONAL_INCLUDES = -I /usr/local/antaris/grpc/include/ ${VENDOR_cJSON_INCLUDES}
INCLUDES = -I /usr/local/include/ -I /usr/lib/antaris/include -I /usr/lib/antaris/gen $(GRPC_CPP_ADDITIONAL_INCLUDES)
LIBS = -L /usr/lib/antaris -lantaris_api -lpthread -lpython3.10 $(GRPC_CPP_ADDITIONAL_LIBS)
############################################################################