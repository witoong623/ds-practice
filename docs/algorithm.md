# Cross line detection
## State that we need
- Line defines by 2 points. Can be multiple lines per camera.
- State per object.
    - Which side it is on the line.
    - How many time does it cross the line?
    - Directions in which it crosses the lines. (can be multiple direction because it can cross multiple times)

## Requirements
- Need to the state for each line.

## Procedure
1. Create reference line along with start and end limit.
2. Get point of the object from bounding box.
    - bbox is rectangle, must find a way to find the point for bbox "reference"
3. Check if point is within line start and end limit.
4. Check which side the point lines on.
5. Depend on the state of the object.
    - If this is the first time, we only know which side it lines on but don't know if it cross yet.
    - If we've seen this object before, and we seen it on the different side than the previous step, it crosses the line.
    - Depend on the previous side state and current side state, we can know the  direction of crossing.
    - If at the current state, the object is on the middle of the line, we consider it as not cross yet,
    and also don't change side of it.
    - If the object goes out of line limits, remove any history about which side it is entirely.
