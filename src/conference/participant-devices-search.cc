/*
 Flexisip, a flexible SIP proxy server with media capabilities.
 Copyright (C) 2018 Belledonne Communications SARL.
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as
 published by the Free Software Foundation, either version 3 of the
 License, or (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.
 
 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "participant-devices-search.hh"


using namespace flexisip;
using namespace std;


ParticipantDevicesSearch::ParticipantDevicesSearch (
	const shared_ptr<linphone::ChatRoom> &cr,
	const shared_ptr<const linphone::Address> &uri
) : mChatRoom(cr), mSipUri(uri) {}

void ParticipantDevicesSearch::run () {
	url_t *url = url_make(mHome.home(), mSipUri->asStringUriOnly().c_str());
	RegistrarDb::get()->fetch(url, shared_from_this(), false, false);
}

void ParticipantDevicesSearch::onRecordFound (Record *r) {
	if (!r)
		return;

	list<shared_ptr<linphone::Address>> listDevices;
	for (const shared_ptr<ExtendedContact> &ec : r->getExtendedContacts()) {
		string uri = ExtendedContact::urlToString(ec->mSipContact->m_url);
		shared_ptr<linphone::Address> addr = linphone::Factory::get()->createAddress(uri);
		if (!addr->getUriParam("gr").empty()
			&& (ec->getOrgLinphoneSpecs().find("groupchat") != string::npos)
		) {
			shared_ptr<linphone::Address> gruuAddr = linphone::Factory::get()->createAddress(
				mSipUri->asStringUriOnly()
			);
			gruuAddr->setUriParam("gr", addr->getUriParam("gr"));
			listDevices.push_back(gruuAddr);
		}
	}

	mChatRoom->setParticipantDevices(mSipUri, listDevices);
}
