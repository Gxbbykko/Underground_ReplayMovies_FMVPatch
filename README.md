# Underground_ReplayMovies_FMVPatch

# ReplayMovies successfully made and created, few bugs still present but those addressed issues will be fixed before the Release version is public, please be patient as this project takes time by one man, the wait will be worth it!

# FMVFix proved working and made
(Xbox Port to PC Port FMV)

# Now the development fix of FMVPatch (FMV Upscale) is in progress, once everything proved and patched, the repository will be updated with everything found/fixed/resolved and final form.

# Xbox vs PC scale parameters ~ They both have the same presentation schedule, but differ in bytes.

```text
decoded Xbox frame: 512 × 256
┌───────────────────────────────┐
│ padding / overscan            │
│   ┌───────────────────────┐   │
│   │ visible movie region  │   │
│   │      480 × 240        │   │
│   └───────────────────────┘   │
│ padding / overscan            │
└───────────────────────────────┘
```

We are now at the better option selection AI Upscale for the FMVs catalogue, because it can upscale the FMV really good than Manual Pixel Upscale, what I'm trying to figure out now before it's permanent, it's in what way it is best for Quality and Performance Wise to target this schedule since I don't wanna affect either Gameplay performance, long Loading times nor FMV desynchronization on this plugin, I wanna make sure it's working smoothly on all 3 sides, this plugin is being worked on besides ThirtheenAG's Widescreen Fix so we can verify and be sure we don't touch his scale plugin but work besides it so we don't brake immersion.
