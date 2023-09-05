if status is-interactive
    # Commands to run in interactive sessions can go here
end

function fish_greeting
end
set -g fish_color_autosuggestion purple
alias api='sudo apt install'
alias aps='sudo apt search'
alias s='sudo'
export PATH=/home/wrq/.nvm/versions/node/v18.14.2/bin:/opt/go/bin:/home/wrq/.cargo/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin:/usr/games:/usr/local/games:/snap/bin:/home/wrq/.local/bin:
fish_add_path /home/wrq/.spicetify
