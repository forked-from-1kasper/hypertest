(local texture₁ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture1.png")))
(local texture₂ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture2.png")))
(local texture₃ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture3.png")))

(local node₁ (hypertest.register hypertest.NODE {:name "Stuff 1" :textures [texture₁ texture₁ texture₁ texture₁ texture₁ texture₁]}))
(local node₂ (hypertest.register hypertest.NODE {:name "Stuff 2" :textures [texture₂ texture₂ texture₂ texture₂ texture₂ texture₂]}))
(local node₃ (hypertest.register hypertest.NODE {:name "Stuff 3" :textures [texture₃ texture₃ texture₃ texture₃ texture₃ texture₃]}))

(hypertest.override
  {:eye     1.62
   :height  1.8
   :gravity 9.8
   :jump    1.25
   :walk    3.0})

(hypertest.setHotbar 0 node₂)
(hypertest.setHotbar 1 node₃)
(hypertest.setHotbar 8 node₁)
