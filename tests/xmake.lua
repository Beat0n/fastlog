-- We need both gtest (the core library) and gtest_main (which provides the main() function)
add_requires("gtest", {configs = {main = true}})

-- Target for MPMC Queue test
target("test_mpmc_queue")
    set_kind("binary")
    add_files("test_mpmc_queue.cpp")
    add_deps("fastlog")
    add_packages("gtest")
    set_targetdir("$(builddir)/bin")

-- Target for Rotating File Sink test
target("test_rotating_file_sink")
    set_kind("binary")
    add_files("test_rotating_file_sink.cpp")
    add_deps("fastlog")
    add_packages("gtest")
    set_targetdir("$(builddir)/bin")
    
    -- RotatingFileSink uses std::filesystem, which might need explicit linking on some toolchains.
    -- xmake usually handles this automatically with set_languages("cxx20"),
    -- but we can add it explicitly for robustness.
    -- add_links("stdc++fs")

-- Target for common type traits and compile-time tests
target("test_common")
    set_kind("binary")
    add_files("test_common.cpp")
    add_deps("fastlog")
    add_packages("gtest")
    set_targetdir("$(builddir)/bin")

-- ... a其他测试目标 ...

-- Target for Pattern Formatter test
target("test_pattern_formatter")
    set_kind("binary")
    add_files("test_pattern_formatter.cpp")
    add_deps("fastlog")
    add_packages("gtest", "fmt") -- Formatter tests might use fmt library directly
    set_targetdir("$(builddir)/bin")

-- Target for MPMC Queue test
target("test_mpmc_queue")
    set_kind("binary")
    add_files("test_mpmc_queue.cpp")
    add_deps("fastlog")
    -- ✅ 确保 gtest 和 fmt 都被添加
    add_packages("gtest", "fmt") 
    set_targetdir("$(builddir)/bin")