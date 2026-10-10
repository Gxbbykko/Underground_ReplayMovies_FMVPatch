# Contributing to the documentation

[← Project overview](README.md) · [Support](SUPPORT.md) · [Security](SECURITY.md)

Thanks for helping improve the accuracy and presentation of this Need for Speed: Underground (2003) engineering archive. The repository currently accepts **documentation-oriented** work; ReplayMovies and FMVFix source trees are published on their respective branches. Proposals must stay within the two authorized source areas; compiled builds and game/movie assets remain excluded.

## Choose the right branch

| Where | Appropriate changes |
| :--- | :--- |
| `main` | Public landing page, project navigation, attribution, repository policies and documentation changelog. |
| `ReplayMovies` | Native menu integration, career catalog, profile progression, lifecycle, validation and history. |
| `FMVFix` | Xbox/PC `.mad` compatibility, parser/decoder investigations, native lifecycle, validation and history. |
| `FMVPatch` | Offline upscaling pipeline, MP4 frame substitution, renderer, synchronization, performance and limitations. |

These branches are **independent documentation areas**; do not propose merging one into another as if they were temporary feature branches.

## Documentation standards

1. **Identify the claim:** give the exact behavior, affected movie or frontend path, and tested executable where applicable.
2. **Name the evidence:** distinguish `runtime-confirmed`, `source/static-verified`, `inferred`, `proposed` and `superseded` results.
3. **Preserve caveats:** e.g. 4K reports cover 15 movies; nine arbitrary-order selections had initial native-MAD fallback; 15/15 concerns a specific ReplayMovies reconstruction test.
4. **Keep filenames and links stable:** edit existing `README.md` and `docs/*.md` paths rather than creating redundant pages or alternate numbered labels.
5. **Write for readers:** begin with purpose; use short sections, tables and focused diagrams; explain acronyms at first use when useful.
6. **Check Markdown:** internal links, tables, code fences and Git whitespace (`git diff --check`) before submitting.

## Do not publish

- Unreviewed third-party or extra `.cpp`, `.h`, `.vcxproj`, `.slnx` files, private source archives or debug symbols outside the exact published source allowlists.
- `.asi`/`.dll` builds, executable game files, original Xbox/PC `.mad` cinematics, upscaled MP4s or raw frame captures.
- Local configuration INIs, personal player profiles, credentials, raw debugger logs or Git database/worktree metadata.
- Unverified claims that a source-only check equals a live gameplay test.

## Submission checklist

- [ ] The change belongs to the selected branch.
- [ ] Links work within that branch or point to the correct named branch URL.
- [ ] Numbers, movie IDs and completion status agree with the final recorded evidence.
- [ ] Historical failures are labeled **superseded** if later records closed them.
- [ ] No private or proprietary artifacts are included.
- [ ] `git diff --check` reports no whitespace errors.

Issue forms and automation are under review; the repository's current published [pull-request template](.github/pull_request_template.md) provides an initial change checklist.
