set(srcs "ipv4.c"
	)

set(requires esp_event esp_stubs protocol_examples_common)


idf_component_register(SRCS "${srcs}"
						INCLUDE_DIRS "."
						)
