idf_component_register(
    # 1. 列出所有 .c 源文件，需要包含 src/ 路径
    SRCS
        "src/bno08x.c"

    # 2. 指定 .h 头文件所在的目录
    INCLUDE_DIRS "inc"

    # 3. 指定依赖的ESP-IDF组件
    REQUIRES driver 
)