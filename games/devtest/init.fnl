(local texture₁ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture1.png")))
(local texture₂ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture2.png")))

(local node₁ (hypertest.register hypertest.NODE {:name "Stuff 1" :textures [texture₁ texture₁ texture₁ texture₁ texture₁ texture₁]}))
(local node₂ (hypertest.register hypertest.NODE {:name "Stuff 2" :textures [texture₂ texture₁ texture₂ texture₂ texture₂ texture₂]}))