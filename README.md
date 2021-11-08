
# Senko3D ![alt tag](https://cdn.discordapp.com/emojis/595690945609990147.png?size=40) 
##### Mafia renderer using sokol_gfx
## Plan
Just experimental repo, playground to get as close as possible to LS3D rendering
- [ ] Fix timing issue ( movement )  
  better dealing with delta time & mouse movement
- [X] Fix black polygons weird bug happens on some models ( solved by CW backface culling )
- [X] Create simple GUI file, get rid of GUI inside game.cpp
- [ ] Bones support 🦴

## Material support:
- [X] Env textures
- [X] Diffuse textures
- [X] Animated diffuse texture
- [ ] Alpha textures
- [ ] Animated alpha textures

### Rendering / visuals
- [ ] Transparency pass ( blending )
- [ ] Glows
- [ ] Fog
- [ ] Lens flare
- [ ] Particle effects ( instancing )
- [X] Env mapping ( not 100% )
- [X] Billboarding ( y axis freeze ) ( needs to fix arbitrary axis )
- [X] Point lights
- [X] Ambient lights
- [X] Dir lights
- [ ] Spot lights
- [ ] Light maps

## Optimalization & oclusion culling 
- [X] AABB From meshes
- [X] Frustum culling
- [X] Sectors ( kinda needs work )
- [X] Single Vertex & Index buffer per model
- [ ] OCTrees
- [ ] Portals
- [ ] Occluders
- [ ] Alternative optimalization ( static batching per sector )
- [ ] Move rendering API specific code into renderer.cpp