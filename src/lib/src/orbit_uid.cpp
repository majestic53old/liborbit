/**
 * liborbit
 * Copyright (C) 2015 David Jolly
 * ----------------------
 *
 * liborbit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * liborbit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "../include/orbit.h"
#include "../include/orbit_uid_type.h"

namespace ORBIT {

	namespace COMPONENT {

		_orbit_uid::_orbit_uid(
			__in_opt orbit_uid_t uid
			) :
				m_uid(uid)
		{
			return;
		}

		_orbit_uid::_orbit_uid(
			__in const _orbit_uid &other
			) :
				m_uid(other.m_uid)
		{
			return;
		}

		_orbit_uid::~_orbit_uid(void)
		{
			return;
		}

		_orbit_uid &
		_orbit_uid::operator=(
			__in const _orbit_uid &other
			)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(this != &other) {
				m_uid = other.m_uid;
			}

			return *this;
		}

		bool 
		_orbit_uid::operator==(
			__in const _orbit_uid &other
			)
		{
			SERIALIZE_CALL_RECUR(m_lock);
			return (m_uid == other.m_uid);
		}

		bool 
		_orbit_uid::operator!=(
			__in const _orbit_uid &other
			)
		{
			SERIALIZE_CALL_RECUR(m_lock);
			return !(*this == other);
		}

		std::string 
		_orbit_uid::to_string(
			__in_opt bool verbose
			)
		{
			std::stringstream result;

			SERIALIZE_CALL_RECUR(m_lock);
			UNREFERENCE_PARAM(verbose);

			result << VALUE_AS_HEX(orbit_uid_t, m_uid);

			return CHECK_STR(result.str());
		}

		orbit_uid_t &
		_orbit_uid::uid(void)
		{
			SERIALIZE_CALL_RECUR(m_lock);
			return m_uid;
		}

		bool 
		operator<(
			__in const _orbit_uid &left,
			__in const _orbit_uid &right
			)
		{
			return (left.m_uid < right.m_uid);
		}

		orbit_uid_factory_ptr orbit_uid_factory::m_instance = NULL;

		_orbit_uid_factory::_orbit_uid_factory(void) :
			m_initialized(false)
		{
			std::atexit(orbit_uid_factory::_delete);
		}

		_orbit_uid_factory::~_orbit_uid_factory(void)
		{

			if(m_initialized) {
				uninitialize();
			}
		}

		void 
		_orbit_uid_factory::_delete(void)
		{

			if(orbit_uid_factory::m_instance) {
				delete orbit_uid_factory::m_instance;
				orbit_uid_factory::m_instance = NULL;
			}
		}				

		orbit_uid_factory_ptr 
		_orbit_uid_factory::acquire(void)
		{

			if(!orbit_uid_factory::m_instance) {

				orbit_uid_factory::m_instance = new orbit_uid_factory;
				if(!orbit_uid_factory::m_instance) {
					THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_ALLOCATION);
				}
			}

			return orbit_uid_factory::m_instance;
		}

		bool 
		_orbit_uid_factory::contains(
			__in const orbit_uid &uid
			)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			return (m_uid_map.find(uid) != m_uid_map.end());
		}

		size_t 
		_orbit_uid_factory::decrement_reference(
			__in const orbit_uid &uid
			)
		{
			size_t result;
			std::map<orbit_uid, size_t>::iterator iter;

			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			iter = find(uid);
			result = --iter->second;
			if(result < REFERENCE_INIT) {
				m_uid_set.insert(iter->first.m_uid);
				m_uid_map.erase(iter);
			}

			return result;
		}

		std::map<orbit_uid, size_t>::iterator 
		_orbit_uid_factory::find(
			__in const orbit_uid &uid
			)
		{
			std::map<orbit_uid, size_t>::iterator result;

			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			result = m_uid_map.find(uid);
			if(result == m_uid_map.end()) {
				THROW_ORBIT_UID_EXCEPTION_MESSAGE(ORBIT_UID_EXCEPTION_NOT_FOUND, 
					"0x%x", uid.m_uid);
			}

			return result;
		}

		orbit_uid 
		_orbit_uid_factory::generate(
			__out size_t &reference
			)
		{
			orbit_uid result;

			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			if(!m_uid_set.empty()) {
				result.uid() = *m_uid_set.begin();
				m_uid_set.erase(result.uid());
			} else if(m_uid_next != UID_INVALID) {
				result.uid() = m_uid_next++;
			} else {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_INSUFFICENT);
			}

			reference = REFERENCE_INIT;
			m_uid_map.insert(std::pair<orbit_uid, size_t>(result, reference));

			return result;
		}

		size_t 
		_orbit_uid_factory::increment_reference(
			__in const orbit_uid &uid
			)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			return ++find(uid)->second;
		}

		void 
		_orbit_uid_factory::initialize(void)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_INITIALIZE);
			}

			m_initialized = true;
			m_uid_map.clear();
			m_uid_next = 0;
			m_uid_set.clear();
		}

		bool 
		_orbit_uid_factory::is_allocated(void)
		{
			return (orbit_uid_factory::m_instance != NULL);
		}

		bool 
		_orbit_uid_factory::is_initialized(void)
		{
			SERIALIZE_CALL_RECUR(m_lock);
			return m_initialized;
		}

		size_t 
		_orbit_uid_factory::reference_count(
			__in const orbit_uid &uid
			)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			return find(uid)->second;
		}

		size_t 
		_orbit_uid_factory::size(void)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			return m_uid_map.size();
		}

		std::string 
		_orbit_uid_factory::to_string(
			__in_opt bool verbose
			)
		{
			size_t index = 0;
			std::stringstream result;
			std::map<orbit_uid, size_t>::iterator iter;

			SERIALIZE_CALL_RECUR(m_lock);

			result << "[" << (m_initialized ? "INIT" : "UNINIT") << "] " 
				<< ORBIT_UID_HEADER;

			if(verbose) {
				result << " (" << VALUE_AS_HEX(uintptr_t, this) << ")";
			}

			for(iter = m_uid_map.begin(); iter != m_uid_map.end(); ++index, ++iter) {
				result << std::endl << "[" << index << "/" << m_uid_map.size() << "] "
					<< VALUE_AS_HEX(orbit_uid_t, iter->first.m_uid) << ", ref: "
					<< iter->second;
			}

			return CHECK_STR(result.str());
		}

		void 
		_orbit_uid_factory::uninitialize(void)
		{
			SERIALIZE_CALL_RECUR(m_lock);

			if(!m_initialized) {
				THROW_ORBIT_UID_EXCEPTION(ORBIT_UID_EXCEPTION_UNINITIALIZE);
			}

			m_uid_map.clear();
			m_uid_next = 0;
			m_uid_set.clear();
			m_initialized = false;
		}
	}
}