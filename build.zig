const std = @import("std");
const gr_audio = @import("gr-audio/build.zig");
const gr_runtime = @import("gnuradio-runtime/build.zig");

const homebrew_prefix = "/opt/homebrew";

pub fn Opt(comptime T: type) type {
    return struct {
        t: type = T,
        name: []const u8,
        description: []const u8,
        default: T,
        value: ?T = null,
    };
}
pub const boolOpts = &[_]Opt(bool){
    .{
        .name = "build_shared",
        .description = "Build shared libraries",
        .default = true,
    },
    .{
        .name = "enable_examples",
        .description = "Enable examples",
        .default = true,
    },
    .{
        .name = "enable_python",
        .description = "Enable python",
        .default = true,
    },
    .{
        .name = "enable_audio",
        .description = "Enable audio",
        .default = false,
    },
};
// const build_shared_libs = b.option(bool, "build_shared", "Build shared libraries") orelse false;
// const enable_examples = b.option(bool, "enable_examples", "Enable examples") orelse false;
// const enable_python = b.option(bool, "enable_python", "Enable python") orelse false;

pub fn buildGNURadioRuntime(b: *std.Build, target: anytype, optimize: anytype) !*std.Build.Step {
    const lib = b.addStaticLibrary(.{
        .name = "gnuradio-runtime",
        .target = target,
        .optimize = optimize,
        .link_libc = true,
    });
    inline for (lib_depts) |d|
        lib.linkSystemLibrary(d);

    b.installArtifact(lib);
    return &lib.step;
}

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    // inline for (boolOpts) |*o| {
    //     o.value = b.option(o.t, o.name, o.description) orelse o.default;
    // }

    const project_name = "gnuradio";
    _ = project_name;
    const testing = true;
    _ = testing;

    // const build_shared_libs = b.option(bool, "build_shared", "Build shared libraries") orelse false;
    // const enable_examples = b.option(bool, "enable_examples", "Enable examples") orelse false;
    // const enable_python = b.option(bool, "enable_python", "Enable python") orelse false;
    // _ = enable_python;
    // _ = enable_examples;
    // _ = build_shared_libs;

    //todo Add more build types and check that requested build type is valid
    //todo # Set the build date
    //todo # Setup Boost for global use (within this build)

    // Distribute the README file
    inline for (distributables) |d|
        b.installFile("./" ++ d, d);

    b.addSearchPrefix(homebrew_prefix);
    inline for (lib_depts) |d| {
        b.addSearchPrefix(homebrew_prefix ++ "/opt/" ++ d);
    }

    inline for (dirs) |d|
        b.addSearchPrefix(d);

    const gnuRadioRuntime = try gr_runtime.buildLib(b, target, optimize);
    const gr_audio_lib = try gr_audio.buildLib(b, target, optimize, gnuRadioRuntime);
    _ = gr_audio_lib;
}

const lib_depts = &.{ "boost", "python" };

const distributables = &.{
    "README.md",
    "CONTRIBUTING.md",
};
const dirs = &.{
    "docs",
    "gnuradio-runtime",
    "gr-blocks",
    "gr-fec",
    "gr-fft",
    "gr-filter",
    "gr-analog",
    "gr-digital",
    "gr-dtv",
    "gr-audio",
    "gr-channels",
    "gr-pdu",
    "gr-iio",
    "gr-qtgui",
    "gr-trellis",
    "gr-uhd",
    "gr-video-sdl",
    "gr-vocoder",
    "gr-wavelet",
    "gr-zeromq",
    "gr-network",
    "gr-soapy",
};
const py_dirs = &.{
    "grc",
    "gr-utils",
};
