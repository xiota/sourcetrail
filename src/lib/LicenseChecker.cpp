#include "LicenseChecker.h"

#include "utility/AppPath.h"
#include "utility/messaging/type/MessageForceEnterLicense.h"
#include "utility/messaging/type/MessageStatus.h"

#include "isTrial.h"
#include "License.h"
#include "PublicKey.h"
#include "settings/ApplicationSettings.h"

void LicenseChecker::createInstance()
{
	if (!s_instance)
	{
		s_instance = std::shared_ptr<LicenseChecker>(new LicenseChecker());
	}
}

std::shared_ptr<LicenseChecker> LicenseChecker::getInstance()
{
	createInstance();

	return s_instance;
}

LicenseChecker::~LicenseChecker()
{
}

std::string LicenseChecker::getCurrentLicenseString() const
{
	License license;
	bool isLoaded = license.loadFromEncodedString(
		ApplicationSettings::getInstance()->getLicenseString(), AppPath::getAppPath());

	if (isLoaded)
	{
		return license.getLicenseString();
	}

	return "";
}

void LicenseChecker::saveCurrentLicenseString(const std::string& licenseString) const
{
	License license;
	bool isLoaded = license.loadFromString(licenseString);
	if (!isLoaded)
	{
		return;
	}

	ApplicationSettings* appSettings = ApplicationSettings::getInstance().get();
	std::string appPath(AppPath::getAppPath());

	appSettings->setLicenseString(license.getLicenseEncodedString(appPath));
	appSettings->setLicenseCheck(license.hashLocation(FilePath(appPath).absolute().str()));
	appSettings->save();
}

bool LicenseChecker::isCurrentLicenseValid()
{
	return checkCurrentLicense() == LICENSE_VALID;
}

LicenseChecker::LicenseState LicenseChecker::checkCurrentLicense() const
{
	ApplicationSettings* appSettings = ApplicationSettings::getInstance().get();

	std::string licenseCheck = appSettings->getLicenseCheck();
	std::string appPath(AppPath::getAppPath());

	std::string licenseString = appSettings->getLicenseString();
	if (licenseString.size() == 0)
	{
		return LICENSE_EMPTY;
	}

	if (!License::checkLocation(FilePath(appPath).absolute().str(), licenseCheck))
	{
		return LICENSE_MOVED;
	}

	License license;
	bool isLoaded = license.loadFromEncodedString(licenseString, appPath);
	if (!isLoaded)
	{
		return LICENSE_MOVED;
	}

	return checkLicense(license);
}

LicenseChecker::LicenseState LicenseChecker::checkLicenseString(const std::string licenseString) const
{
	if (licenseString.size() == 0)
	{
		return LICENSE_EMPTY;
	}

	License license;
	bool isLoaded = license.loadFromString(licenseString);
	if (!isLoaded)
	{
		return LICENSE_MALFORMED;
	}

	license.print();

	return checkLicense(license);
}

LicenseChecker::LicenseChecker()
	: m_forcedLicenseEntering(false)
{
}

void LicenseChecker::handleMessage(MessageDispatchWhenLicenseValid* message)
{
	if (!isTrial())
	{
		MessageStatus("preparing...", false, true).dispatch();

		LicenseState state = checkCurrentLicense();

		MessageStatus("ready").dispatch();

		if (state != LICENSE_VALID)
		{
			m_pendingMessage = message->content;

			if (!m_forcedLicenseEntering)
			{
				m_forcedLicenseEntering = true;
				MessageForceEnterLicense(state == LICENSE_EXPIRED).dispatch();
			}

			return;
		}
	}

	message->content->dispatch();
}

void LicenseChecker::handleMessage(MessageEnteredLicense* message)
{
	m_forcedLicenseEntering = false;

	if (m_pendingMessage)
	{
		m_pendingMessage->dispatch();
		m_pendingMessage.reset();
	}
}

LicenseChecker::LicenseState LicenseChecker::checkLicense(License& license) const
{
	license.loadPublicKeyFromString(PublicKey);

	if (license.isExpired())
	{
		return LICENSE_EXPIRED;
	}

	if (license.isValid())
	{
		return LICENSE_VALID;
	}

	return LICENSE_INVALID;
}

std::shared_ptr<LicenseChecker> LicenseChecker::s_instance;