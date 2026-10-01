Optional sound files (any format libsndfile understands: .wav / .ogg / .flac).
If a file is missing, the game synthesises a small retro "blip" instead, so
nothing crashes and the game is never silent by accident.

  shoot.wav          - heroine fires her power
  enemy_death.wav    - an enemy is defeated
  player_hit.wav     - the heroine gets hurt
  player_death.wav   - the heroine is defeated
  pickup.wav         - heart pickup collected

Optional font for on-screen text: assets/font.ttf (any TrueType font).
If absent, the game tries common system fonts and otherwise runs text-free.
