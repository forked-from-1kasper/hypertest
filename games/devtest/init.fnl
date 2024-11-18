(local texture₁ (core.register core.TEXTURE (.. core.dirname "/textures/texture1.png")))
(local texture₂ (core.register core.TEXTURE (.. core.dirname "/textures/texture2.png")))
(local texture₃ (core.register core.TEXTURE (.. core.dirname "/textures/texture3.png")))

(local node₁ (core.register core.NODE {:name "Stuff 1" :textures [texture₁ texture₁ texture₁ texture₁ texture₁ texture₁]}))
(local node₂ (core.register core.NODE {:name "Stuff 2" :textures [texture₂ texture₂ texture₂ texture₂ texture₂ texture₂]}))
(local node₃ (core.register core.NODE {:name "Stuff 3" :textures [texture₃ texture₃ texture₃ texture₃ texture₃ texture₃]}))

(core.override
  {:eye     1.62
   :height  1.8
   :gravity 9.8
   :jump    1.25
   :walk    3.0})

(core.setHotbar 0 node₂)
(core.setHotbar 1 node₃)
(core.setHotbar 8 node₁)
