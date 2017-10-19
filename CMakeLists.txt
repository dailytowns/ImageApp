cmake_minimum_required(VERSION 3.8)
project(ImageApp)

set(CMAKE_C_STANDARD 90)

set(SOURCE_FILES HttpServer.c Utils.c ThreadPool.c Log.c Message.c
                HandleImage.c HandleDB.c Strings.c treexml.pgc include)
set(SOURCE_FILES_XML ParseWurfl.c Utils.c)

add_executable(httpserver ${SOURCE_FILES})

add_executable(ParseWurfl ${SOURCE_FILES_XML})

target_link_libraries(httpserver pthread)


###### PkgConfig ########
find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBXML2 "libxml-2.0")
    if(LIBXML2_FOUND)
        #target_link_libraries(httpserver ${LIBXML2_LIBRARIES})
        target_link_libraries(ParseWurfl ${LIBXML2_LIBRARIES})
        add_definitions(${LIBXML2_CFLAGS} ${LIBXML2_CFLAGS_OTHER})
    endif()
    pkg_check_modules(IMAGEMAGICK "MagickWand")
    if(IMAGEMAGICK_FOUND)
        target_link_libraries(httpserver ${IMAGEMAGICK_LIBRARIES})
            #target_link_libraries(TEST ${IMAGEMAGICK_LIBRARIES})
        add_definitions(${IMAGEMAGICK_CFLAGS} ${IMAGEMAGICK_CFLAGS_OTHER})
    endif()
    #[[pkg_check_modules(ECPG "libecpg")
    if(ECPG_FOUND)
        #target_link_libraries(httpserver ${ECPG_LIBRARIES})
        target_link_libraries(treexml ${ECPG_LIBRARIES})
        add_definitions(${ECPG_CFLAGS} ${ECPG_CFLAGS_OTHER})
    endif()]]
    pkg_check_modules(LIBPQ "libpq")
    if(LIBPQ_FOUND)
        target_link_libraries(httpserver ${LIBPQ_LIBRARIES})
        add_definitions(${LIBPQ_CFLAGS} ${LIBPQ_CFLAGS_OTHER})
    endif()
    pkg_check_modules(LIBMYSQLCLIENT "mysqlclient")
    if(LIBMYSQLCLIENT_FOUND)
        target_link_libraries(httpserver ${LIBMYSQLCLIENT_LIBRARIES})
        target_link_libraries(ParseWurfl ${LIBMYSQLCLIENT_LIBRARIES})
        #target_link_libraries(MINIM ${LIBMYSQLCLIENT_LIBRARIES})
        add_definitions(${LIBMYSQLCLIENT_CFLAGS} ${LIBMYSQLCLIENT_CFLAGS_OTHER})
    endif()
endif()
#######################