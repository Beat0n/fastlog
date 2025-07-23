-- This script describes how to build the benchmarks.
-- It will be included by the root xmake.lua.

-- Add the google-benchmark package from the xmake-repo repository.
-- xmake will automatically handle downloading and building it.
add_requires("benchmark")

target("bench_mpmc_queue")
    -- We are building an executable program.
    set_kind("binary")

    -- Add the source file for this specific benchmark.
    add_files("bench_mpmc_queue.cpp")

    -- This benchmark executable DEPENDS ON our core "flog" library,
    -- because it needs to access the mpmc_queue.h header
    -- and potentially other parts of our library.
    add_deps("fastlog")

    -- Add the packages (dependencies) to this target.
    add_packages("benchmark")

    -- After building, place the executable in the 'build/bin' directory for clarity.
    set_targetdir("$(builddir)/bin")

-- ... bench_mpmc_queue target ...

-- Target for the full backend benchmark
target("bench_backend")
    set_kind("binary")
    add_files("bench_backend.cpp")
    add_deps("fastlog")
    add_packages("benchmark", "fmt")
    set_targetdir("$(builddir)/bin")