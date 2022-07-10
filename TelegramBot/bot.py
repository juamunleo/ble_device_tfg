from telegram.ext.updater import Updater
from telegram.update import Update
from telegram.ext.callbackcontext import CallbackContext
from telegram.ext.commandhandler import CommandHandler
from telegram.ext.messagehandler import MessageHandler
from telegram.ext.filters import Filters
from flask import Flask, request

app = Flask(__name__)

@app.route('/', methods=['POST'])
def post():
    sendNotification(request.form['name'], request.form['locationLink'])
    return 'Received!'

with open('bot_token.txt') as f: #Crear un archivo bot_token.txt donde la primera línea tiene el token del bot.
    lines = f.readlines()
    token = lines[0]
    
updater = Updater(token, use_context=True)

users = []
  
def start(update: Update, context: CallbackContext):
    if(not users.__contains__(update.effective_user)):
        update.message.reply_text("Has sido añadido a la lista de notificados.")
        users.append(update.effective_user)
    else:
        update.message.reply_text("Ya estabas en la lista de notificados.")

def help(update: Update, context: CallbackContext):
    update.message.reply_text(""" Comandos :
    /start - Añadir tu usuario a la lista de notificados
    /help - Muestra este menú""")

def sendNotification(name, locationLink):
    for user in users:
        user.send_message(name + " ha enviado una notificación desde esta ubicación: " + locationLink)

updater.dispatcher.add_handler(CommandHandler('start', start))
updater.dispatcher.add_handler(CommandHandler('help', help))

updater.start_polling()

app.run(host="0.0.0.0", port=8080, debug=False)