The program implements loop's subdvision mask using mask convolution. There were 2 main assignments:

1. Add boundary rules for Loop subdivision
The implementation failed to set next and prev for boundary edges. This was improved in meshtools.cpp by looping over the boundary edges and updating the next and prev.

2.  Implement reflection lines or isophotes
Reflection lines was implemented by using a reflectionshader. The angle is calculated and using that angle a color is assigned alternating between black and white. 