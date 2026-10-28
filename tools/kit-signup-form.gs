// HOT Info Orbs kit sign-up form. Paste into a new Apps Script project
// (script.google.com) and run buildKitSignupForm. It creates the form and a
// linked responses Sheet in the running account's Drive, then logs the links.
// Running it again makes a second form; edit the existing one in Forms instead.
// Built 2026-10-28; live form:
// https://docs.google.com/forms/d/e/1FAIpQLSewJmwJH4fZf1O3rndbRK9dUFfkHtH9RUs-GQdB2Rw4AMjylw/viewform

const LIVE_FORM_ID = '1MMD9PAcmM4eElS5t4GJQU5yMvdGBJt0DDh9DqYVrQhc';

const DESCRIPTION =
  'Five round displays on your desk showing the time, the weather and the markets. ' +
  'It updates over Wi-Fi and you set it up from any browser.\n\n' +
  'Through-hole soldering only. Kits are handed out at the November meeting; build yours ' +
  'before the January meeting. If you want help, there are two catch-up meetings in between.\n\n' +
  "WHAT'S IN THE KIT: $50\n" +
  '\u2022 The carrier board\n' +
  '\u2022 A Seeed XIAO ESP32-S3\n' +
  '\u2022 Five 1.28-inch round displays\n' +
  '\u2022 Sockets for the displays and the XIAO, plus header pins\n' +
  '\u2022 Three buttons\n' +
  '\u2022 Two capacitors and a resistor\n' +
  '\u2022 3D print files for the enclosure (3MF)\n\n' +
  'YOU SUPPLY\n' +
  "\u2022 A 90\u00B0 (right-angle) USB-C data cable. We'll send the exact spec. " +
  "A charge-only cable won't load the firmware.\n\n" +
  'Feature sheet: https://scottdube.github.io/hot-info-orbs/features.html';

const CONFIRMATION = "Thanks, you're on the list. We'll email how to pay.";

const RENDER_URL =
  'https://raw.githubusercontent.com/scottdube/hot-info-orbs/main/docs/assets/enclosure-concept-3-winged-speedster.png';
const RENDER_TITLE = 'The Winged Speedster enclosure. 3D print files (3MF) are provided.';

const KIT_COUNTS = ['1', '2', '3', '4'];
const ENCLOSURE_HELP = 'The 3D print files (3MF) come with the kit.';
const ENCLOSURE_CHOICES =
  ["I'll print my own", "I'll need to find someone to print it for me", 'No enclosure'];
const SKILL_TITLE = 'Soldering skill';
const SKILL_CHOICES = [
  'Never tried it',
  'I burn traces and the board regularly',
  'OK, but could use help',
  'I feel confident I can do this',
  'I do SMD work with my eyes closed',
  'I can help others',
];
const PAYMENT_TITLE = 'Payment';
const PAYMENT_TEXT =
  "By clicking Submit, you agree to pay $50 per kit so we can buy the parts on your behalf. " +
  "We'll email how to pay.";

function addRender_(form) {
  const blob = UrlFetchApp.fetch(RENDER_URL).getBlob();
  const image = form.addImageItem().setImage(blob).setTitle(RENDER_TITLE).setAlignment(FormApp.Alignment.CENTER);
  form.moveItem(image.getIndex(), 0);
}

function buildKitSignupForm() {
  const form = FormApp.create('HOT Info Orbs: Kit Sign-up');
  form.setDescription(DESCRIPTION);
  form.setConfirmationMessage(CONFIRMATION);
  form.setAllowResponseEdits(true);
  form.setProgressBar(false);

  form.addTextItem().setTitle('First name').setRequired(true);
  form.addTextItem().setTitle('Last name').setRequired(true);

  form.addTextItem()
    .setTitle('Email')
    .setRequired(true)
    .setValidation(FormApp.createTextValidation()
      .requireTextIsEmail()
      .setHelpText('Please enter a valid email address.')
      .build());

  form.addTextItem().setTitle('Phone').setRequired(true);

  form.addListItem()
    .setTitle('How many kits?')
    .setChoiceValues(KIT_COUNTS)
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle('Enclosure')
    .setHelpText(ENCLOSURE_HELP)
    .setChoiceValues(ENCLOSURE_CHOICES)
    .setRequired(true);

  form.addMultipleChoiceItem()
    .setTitle(SKILL_TITLE)
    .setChoiceValues(SKILL_CHOICES)
    .setRequired(true);

  form.addParagraphTextItem().setTitle('Questions or comments');

  // Forms has no text beside the Submit button; the last item sits just above it.
  form.addSectionHeaderItem().setTitle(PAYMENT_TITLE).setHelpText(PAYMENT_TEXT);

  addRender_(form);

  // A Workspace account can default new forms to its own domain; members
  // sign up from personal addresses, so open it to anyone with the link.
  try { form.setRequireLogin(false); } catch (e) { Logger.log('setRequireLogin: ' + e); }
  // Forms made from a script may start unpublished.
  try { form.setPublished(true); } catch (e) { Logger.log('setPublished: ' + e); }

  const sheet = SpreadsheetApp.create('HOT Info Orbs: Kit Sign-up (Responses)');
  form.setDestination(FormApp.DestinationType.SPREADSHEET, sheet.getId());

  Logger.log('Share:     ' + form.getPublishedUrl());
  Logger.log('Edit:      ' + form.getEditUrl());
  Logger.log('Responses: ' + sheet.getUrl());
}

// Bring the live form to the design above without changing its link.
// Safe to rerun: it matches items by title and adds missing ones once.
function updateLiveForm() {
  const form = FormApp.openById(LIVE_FORM_ID);
  form.setDescription(DESCRIPTION);
  form.setConfirmationMessage(CONFIRMATION);

  form.getItems().forEach(function (item) {
    const title = item.getTitle();
    if (title === 'Module') {
      form.deleteItem(item);
    } else if (title === 'Name') {
      item.setTitle('First name');
    } else if (title === 'Phone') {
      item.asTextItem().setRequired(true).setHelpText('');
    } else if (title === 'How many kits?') {
      item.asListItem().setChoiceValues(KIT_COUNTS);
    } else if (title === 'Enclosure') {
      item.asMultipleChoiceItem().setHelpText(ENCLOSURE_HELP).setChoiceValues(ENCLOSURE_CHOICES);
    } else if (title === 'Soldering experience' || title === SKILL_TITLE) {
      item.asMultipleChoiceItem().setTitle(SKILL_TITLE).setHelpText('').setChoiceValues(SKILL_CHOICES);
    }
  });

  const titles = form.getItems().map(function (item) { return item.getTitle(); });
  if (titles.indexOf('Last name') === -1) {
    const last = form.addTextItem().setTitle('Last name').setRequired(true);
    form.moveItem(last.getIndex(), titles.indexOf('First name') + 1);
  }
  if (titles.indexOf(PAYMENT_TITLE) === -1) {
    form.addSectionHeaderItem().setTitle(PAYMENT_TITLE).setHelpText(PAYMENT_TEXT);
  }

  const images = form.getItems(FormApp.ItemType.IMAGE);
  if (images.length === 0) addRender_(form);
  else images[0].setTitle(RENDER_TITLE);

  form.getItems().forEach(function (item, i) { Logger.log(i + ' ' + item.getType() + ' ' + item.getTitle()); });
}
