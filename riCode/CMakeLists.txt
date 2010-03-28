# Information Retrieval Collection Parser Project
# author: Anisio Mendes Lacerda
# mail: anisio@dcc.ufmg.br

PROJECT(IRCP)

set (CMAKE_C_FLAGS "-Wall -Werror")
set (CMAKE_CXX_FLAGS "-Wall -Werror")

ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( test )

