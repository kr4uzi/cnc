#include "server_session.h"
#include <curses.h>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <vector>
#include <string>
#include <functional>
#include <thread>
using namespace cnc;

class console
{
	boost::asio::io_context &m_context;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> m_work;
	bool m_stopped = false;
	WINDOW *m_tabs;
	WINDOW *m_tab_border;
	WINDOW *m_logs;
	WINDOW *m_log_border;
	WINDOW *m_input;
	std::thread m_thread;

public:
	std::vector<std::string> tabs;
	bool tabs_changed = false;
	std::vector<std::string> logs;
	bool logs_changed = false;
	std::string input;
	bool input_changed = false;

	boost::signals2::signal<void(int)> on_input;

	console(boost::asio::io_context &context)
		: m_context(context), m_work(boost::asio::make_work_guard(context))
	{
		initscr();
		clear();
		noecho();
		cbreak();

		m_tabs = newwin(1, COLS, 0, 0);

		m_tab_border = newwin(1, COLS, 1, 0);
		mvwhline(m_tab_border, 0, 0, '\0', COLS);
		wrefresh(m_tab_border);

		m_logs = newwin(LINES - 4, COLS, 2, 0);
		scrollok(m_logs, TRUE);

		m_log_border = newwin(1, COLS, LINES - 2, 0);
		mvwhline(m_log_border, 0, 0, '\0', COLS);
		wrefresh(m_log_border);

		m_input = newwin(1, COLS, LINES - 1, 0);
		keypad(m_input, TRUE);

		m_thread = std::thread([&]
		{
			while (m_work.owns_work())
			{
				int key = wgetch(m_input);
				if (key != ERR && m_work.owns_work())
					m_context.dispatch([&] { on_input(key); });
			}
		});
	}

	void draw()
	{
		if (tabs_changed)
		{
			wclear(m_tabs);
			wmove(m_tabs, 0, 0);

			for (const auto &name : tabs)
			{
				for (const auto &ch : name)
					waddch(m_tabs, ch);

				whline(m_tabs, '\0', 1);
			}

			wrefresh(m_tabs);
			tabs_changed = false;
		}

		if (logs_changed)
		{
			wclear(m_logs);
			wmove(m_logs, 0, 0);

			for (std::size_t i = 0; i < logs.size(); i++)
			{
				for (const auto &ch : logs[i])
					waddch(m_logs, ch);

				if (i != logs.size() - 1)
					waddch(m_logs, '\n');
			}

			wrefresh(m_logs);
			logs_changed = false;
		}

		if (input_changed)
		{
			wclear(m_input);
			wmove(m_input, 0, 0);

			for (const auto &ch : input)
				waddch(m_input, ch);

			wrefresh(m_input);
			input_changed = false;
		}
	}

	bool stopped() const noexcept { return m_stopped; }

	void stop()
	{
		if (m_stopped)
			return;

		m_stopped = true;
		nodelay(m_input, TRUE);
		m_work.reset();
		if (m_thread.joinable())
			m_thread.join();
	}

	~console()
	{
		stop();
		delwin(m_input);
		delwin(m_log_border);
		delwin(m_logs);
		delwin(m_tab_border);
		delwin(m_tabs);
		endwin();
	}

	void resize()
	{
		wresize(m_tabs, 1, COLS);
		mvwin(m_tabs, 0, 0);
		tabs_changed = true;

		wresize(m_tab_border, 1, COLS);
		mvwin(m_tab_border, 1, 0);
		mvwhline(m_tab_border, 0, 0, '\0', COLS);

		wresize(m_logs, LINES - 4, COLS);
		mvwin(m_logs, 2, 0);

		wresize(m_log_border, 1, COLS);
		mvwin(m_log_border, LINES - 4, 0);
		mvwhline(m_log_border, 0, 0, '\0', COLS);

		wresize(m_input, 1, COLS);
		mvwin(m_input, LINES - 5, 0);
	}
};

console &operator<<(console &con, const std::string &cmd)
{
	con.logs.push_back(cmd);
	return con;
}

int main()
{
	boost::asio::io_context ctx;
	console con(ctx);
	server_session session(ctx, boost::asio::ip::make_address("127.0.0.1"), 8081);


	session.on_hello.connect([&](auto data)
	{
		
	});

	std::vector<std::string> history;
	std::size_t history_iter = 0;

	con.on_input.connect([&](int c)
	{
		if (c == KEY_RESIZE)
			con.resize();
		else if (c == 3)
		{
			con.input.clear();
			con.input_changed = true;
		}
		else if (c == 8 || c == 127 || c == KEY_BACKSPACE || c == KEY_DC)
		{
			if (!con.input.empty())
			{
				con.input.erase(con.input.size() - 1, 1);
				con.input_changed = true;
			}
		}
		else if (c == 9)
			; // tab
		else if (c == 10 || c == 13)
		{
			std::string cmd = con.input;
			if (!cmd.empty())
			{
				history.push_back(cmd);
				history_iter = history.size();

				con.logs.push_back(cmd);
				con.logs_changed = true;

				con.input.clear();
				con.input_changed = true;

				if (cmd == "/help")
				{
					con << " /help                   : prints this message";
					con << " /server <ip> <port>     : connect to server";
					con << " /relay <mac>            : request client to connect to this machine";
					con << " /dir <path>             : list directory of <path>";
					con << " /exec <executable>      : execute <executable> print result";
					con.logs_changed = true;
				}
			}
		}
		else if(32 <= c && c <= 255)
		{
			con.input += c;
			con.input_changed = true;
		}
		else if (c == KEY_UP || c == KEY_DOWN)
		{
			if (c == KEY_UP && 0 < history_iter && history_iter <= history.size())			
				history_iter--;
			else if (c == KEY_DOWN && 0 <= history_iter && history_iter <= history.size())
			{
				history_iter++;
				if (history_iter >= history.size())
				{
					history_iter = history.size();
					con.input.clear();
					goto skip;
				}
			}

			con.input = history[history_iter];
		skip:
			con.input_changed = true;			
		}
		else
		{
			con.logs.push_back(std::to_string(c));
			con.logs_changed = true;
		}

		con.draw();		
	});

	ctx.run();
}