-- We need both gtest (the core library) and gtest_main (which provides the main() function)
add_requires("gtest", {configs = {main = true}})

-- Target for MPMC Queue test
target("test_mpmc_queue")
    set_kind("binary")
    add_files("test_mpmc_queue.cpp")
    add_deps("fastlog")
    add_packages("gtest")
    set_targetdir("$(buildir)/bin")

-- Target for Rotating File Sink test
target("test_rotating_file_sink")
    set_kind("binary")
    add_files("test_rotating_file_sink.cpp")
    add_deps("fastlog")
    add_packages("gtest")
    set_targetdir("$(buildir)/bin")
    
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
    set_targetdir("$(buildir)/bin")