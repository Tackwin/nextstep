
#define struct_A242(x, y)\
	template<size_t H> struct x##_;\
	using x = x##_<hash(#x)>;\
	template<size_t H> struct x##_ : y##_<H>

template<size_t H>
struct id_t {
	static constexpr u32 hash = (u32)H;
	u32 id = (u32)H;
};
template<>
struct id_t<0> {
};

template<size_t H>
struct Representation_Item_ : id_t<H> {
	Read_String name;
};
using Representation_Item = Representation_Item_<0>;

struct_A242(Geometric_Representation_Item, Representation_Item) {

};

struct_A242(Point, Geometric_Representation_Item) {
};
struct_A242(Cartesian_Point, Point) {
	float x;
	float y;
	float z;
};
struct_A242(Placement, Geometric_Representation_Item) {
	ref<Cartesian_Point> location;
};
struct_A242(Direction, Geometric_Representation_Item) {
	float x;
	float y;
	float z;
};
struct_A242(Axis2_Placement_3d, Placement) {
	Direction* axis = nullptr;
	Direction* ref_direction = nullptr;
};

struct_A242(Surface, Geometric_Representation_Item) {
};
struct_A242(Elementary_Surface, Surface) {
	ref<Axis2_Placement_3d> position = nullptr;
};
struct_A242(Plane, Elementary_Surface) {
};

#undef struct_A242