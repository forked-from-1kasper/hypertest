#include <Hyper/Config.hxx>

Config::Config(LuaJIT * luajit, const char * filename) {
    if (LuaTable config = luajit->require(filename)) {
        if (LuaString world_v = config.getitem("world"))
            world = world_v.decode();

        if (LuaTable window_v = config.getitem("window")) {
            if (LuaInteger width_v = window_v.getitem("width"))
                window.width = width_v.decode();

            if (LuaInteger height_v = window_v.getitem("height"))
                window.height = height_v.decode();

            if (LuaInteger msaa_v = window_v.getitem("msaa"))
                window.msaa = msaa_v.decode();
        }

        if (LuaTable camera_v = config.getitem("camera")) {
            if (LuaNumber crd_v = camera_v.getitem("chunkRenderDistance"))
                camera.chunkRenderDistance = crd_v.decode();

            if (LuaNumber fov_v = camera_v.getitem("fov"))
                camera.fov = fov_v.decode();

            if (LuaNumber near_v = camera_v.getitem("near"))
                camera.near = near_v.decode();

            if (LuaNumber far_v = camera_v.getitem("far"))
                camera.far = far_v.decode();

            if (LuaInteger model_v = camera_v.getitem("model"))
                camera.model = Model(model_v.decode());
        }

        if (LuaTable fog_v = config.getitem("fog")) {
            if (LuaBool enabled_v = fog_v.getitem("enabled"))
                fog.enabled = enabled_v.decode();

            if (LuaNumber near_v = fog_v.getitem("near"))
                fog.near = near_v.decode();

            if (LuaNumber far_v = fog_v.getitem("far"))
                fog.far = far_v.decode();

            if (LuaVec4 color_v = fog_v.getitem("color"))
                fog.color = color_v.decode();
        }

        if (LuaTable gui_v = config.getitem("gui")) {
            if (LuaNumber aim_v = gui_v.getitem("aimSize"))
                gui.aimSize = aim_v.decode();
        }
    }
}