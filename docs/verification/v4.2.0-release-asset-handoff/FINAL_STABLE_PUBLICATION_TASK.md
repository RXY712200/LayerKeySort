# Publish LayerKeySort v4.2.0 Stable

The repository owner has explicitly authorized publication. **Publish directly. Do not ask for another confirmation and do not repeat the lengthy asset audit.**

Repository: `RXY712200/LayerKeySort`

Target commit: `f751e1ee2afd682874281d6dc495969f9eca49eb`

Target tree: `18261736d37cba995a20d9a3132313d954ff3ef1`

Prepared publication directory:

`D:\LayerKeySort\.local\full-v4.2.0-final-f751e1e\build\v4.2.0-publication\`

## Do the release now

1. Read the prepared `PUBLICATION.json`, `commands.json`, `RELEASE_NOTES.en.md`, and `RELEASE_NOTES.zh-CN.md` in the publication directory. Use the prepared release notes.
2. Confirm only the essentials: `main` still points to the target commit above, and `v4.2.0` tag/Release do not already exist. If so, continue immediately.
3. Create and push the annotated tag:
   `git tag -a v4.2.0 f751e1ee2afd682874281d6dc495969f9eca49eb -m "LayerKeySort v4.2.0"`
   `git push origin refs/tags/v4.2.0`
4. Publish the GitHub Release:
   - Title: `LayerKeySort v4.2.0`
   - Tag: `v4.2.0`
   - Published, not draft
   - Not a prerelease
   - Set as Latest
   - Use the prepared English and Chinese release notes.
5. Upload exactly these two already-prepared files. Do not rebuild or repackage them:
   - `LayerKeySort-4.2.0-amalgamation.zip`
   - `LayerKeySort-4.2.0-SHA256SUMS.txt`
6. Confirm the Release is published, the tag points to the target commit, both assets appear, and v4.2.0 is Latest. Record the Release URL and final result in a short publication receipt under the existing handoff directory and commit that receipt to this task branch only.

## Scope

This is the authorized final publication. Finish the steps above in one continuous task. Do not pause for another owner approval, add unnecessary review stages, or repeat the full independent ZIP audit.

Do not change source code, modify `main`, merge branches, create a PR, or alter previous tags/releases. If an essential precondition unexpectedly changed, report the exact issue rather than overwriting existing publication state.

## Final report

Report the published Release URL, tag target, Latest status, and the two uploaded assets. State plainly if any step could not be completed.
