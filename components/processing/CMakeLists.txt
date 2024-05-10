set(srcs "preproc.c" "postproc.c" "proc_utils.c"
	)



idf_component_register(SRCS "${srcs}"
						INCLUDE_DIRS "."
						)
