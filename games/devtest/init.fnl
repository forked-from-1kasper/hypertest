(local texture₁ (core.register core.TEXTURE 0xEEEEEEFF))
(local texture₂ (core.register core.TEXTURE 0x00FF00FF))
(local texture₃ (core.register core.TEXTURE 0x0000FFFF))
(local texture₄ (core.register core.TEXTURE 0xFF0000FF))

(local node₁ (core.register core.NODE {:name "Stuff 1" :textures [texture₁ texture₁ texture₁ texture₁ texture₁ texture₁]}))
(local node₂ (core.register core.NODE {:name "Stuff 2" :textures [texture₂ texture₂ texture₂ texture₂ texture₂ texture₂]}))
(local node₃ (core.register core.NODE {:name "Stuff 3" :textures [texture₃ texture₃ texture₃ texture₃ texture₃ texture₃]}))
(local node₄ (core.register core.NODE {:name "Stuff 4" :textures [texture₄ texture₄ texture₄ texture₄ texture₄ texture₄]}))

(core.override
  {:eye     1.62
   :height  1.8
   :gravity 9.8
   :jump    1.25
   :walk    3.0})

(core.setHotbar 0 node₂)
(core.setHotbar 1 node₃)
(core.setHotbar 2 node₄)
(core.setHotbar 8 node₁)

(core.background 1.0 1.0 1.0 1.0)
