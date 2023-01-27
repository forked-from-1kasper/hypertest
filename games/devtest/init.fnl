(local texture₁ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture1.png")))
(local texture₂ (hypertest.register hypertest.TEXTURE (.. hypertest.dirname "/textures/texture2.png")))

(local node₁ (hypertest.register hypertest.NODE {:name "Stuff 1" :textures [texture₁ texture₁ texture₁ texture₁ texture₁ texture₁]}))
(local node₂ (hypertest.register hypertest.NODE {:name "Stuff 2" :textures [texture₂ texture₂ texture₂ texture₂ texture₂ texture₂]}))

(hypertest.override
  {:eye     1.62
   :height  1.8
   :gravity 9.8
   :jump    1.25
   :walk    3.0})