#include "caching/mapping_array.h"
#include "persistent-data/endian_utils.h"

using namespace caching;
using namespace caching::mapping_array_damage;

//----------------------------------------------------------------

namespace {
	const uint64_t FLAGS_MASK = (1 << 16) - 1;
}

void
mapping_traits::unpack(disk_type const &disk, value_type &value)
{
	uint64_t v = base::to_cpu<uint64_t>(disk);
	value.oblock_ = v >> 16;
	value.flags_ = v & FLAGS_MASK;
}

void
mapping_traits::pack(value_type const &value, disk_type &disk)
{
	uint64_t packed = value.oblock_ << 16;
	packed = packed | (value.flags_ & FLAGS_MASK);
	disk = base::to_disk<le64>(packed);
}

//----------------------------------------------------------------

missing_mappings::missing_mappings(std::string const &desc, run<uint32_t> const &keys)
	: damage(desc),
	  keys_(keys)
{
}

void
missing_mappings::visit(damage_visitor &v) const
{
	v.visit(*this);
}

invalid_mapping::invalid_mapping(std::string const &desc,
				 block_address cblock, mapping const &m)
	: damage(desc),
	  cblock_(cblock),
	  m_(m)
{
}

void
invalid_mapping::visit(damage_visitor &v) const
{
	v.visit(*this);
}

namespace {
	struct no_op_visitor {
		virtual void visit(uint32_t index,
				   mapping_traits::value_type const &v) {
		}
	};

	class ll_damage_visitor {
	public:
		ll_damage_visitor(damage_visitor &v)
		: v_(v) {
		}

		virtual void visit(array_detail::damage const &d) {
			v_.visit(missing_mappings(d.desc_, d.lost_keys_));
		}

	private:
		damage_visitor &v_;
	};
}

void
caching::check_mapping_array(mapping_array const &array, damage_visitor &visitor)
{
	no_op_visitor vv;
	ll_damage_visitor ll(visitor);
	array.visit_values(vv, ll);
}

//----------------------------------------------------------------
