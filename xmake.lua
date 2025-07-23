set_project("fastlog")

add_rules("mode.debug", "mode.release")

set_languages("c++20")

add_requires("fmt")

-- Add default build modes (debug, release)
add_rules("mode.debug", "mode.release", {public = true})

-- Set the output directory for all build artifacts (binaries, libraries, etc.)
-- This keeps the source tree clean.
set_targetdir("build")

target("fastlog")
    -- 1. 库的类型"static"
    set_kind("static")

    add_packages("fmt", {public = true})

    -- 添加所有需要编译的源文件
    -- 我们使用通配符来包含 src/ 及其所有子目录下的 .cpp 文件
    add_files("src/**.cpp")

    -- 为库本身和所有依赖方提供头文件路径
    add_includedirs("include", {public = true})


includes("tests", "bench", "examples")